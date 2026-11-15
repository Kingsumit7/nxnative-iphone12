import SwiftUI
import AppKit
import Combine

private struct GameSource: Identifiable, Codable, Hashable {
    var id: UUID
    var name: String
    var path: String
    var addedAt: Date

    var url: URL { URL(fileURLWithPath: path) }
}

@MainActor
private final class LauncherModel: ObservableObject {
    @Published var games: [GameSource] = []
    @Published var selectedGameID: UUID?
    @Published var repositoryPath: String
    @Published var keysPath: String
    @Published var outputPath: String
    @Published var log = ""
    @Published var status = "Ready"
    @Published var isBuilding = false
    @Published var progressFraction: Double?
    @Published var progressLabel = "Ready"
    @Published var errorMessage: String?

    private var process: Process?
    private var pendingProgressLine = ""

    private let gamesKey = "library.games"
    private let repositoryKey = "repository.path"
    private let keysKey = "keys.path"
    private let outputKey = "output.path"

    init() {
        repositoryPath = UserDefaults.standard.string(forKey: repositoryKey) ?? ""
        keysPath = UserDefaults.standard.string(forKey: keysKey) ?? ""
        outputPath = UserDefaults.standard.string(forKey: outputKey)
            ?? FileManager.default.homeDirectoryForCurrentUser.appendingPathComponent("NXNative Builds").path

        if let data = UserDefaults.standard.data(forKey: gamesKey),
           let saved = try? JSONDecoder().decode([GameSource].self, from: data) {
            games = saved.filter { FileManager.default.fileExists(atPath: $0.path) }
        }
        selectedGameID = games.first?.id
    }

    var selectedGame: GameSource? {
        games.first { $0.id == selectedGameID }
    }

    func addSources() {
        let panel = NSOpenPanel()
        panel.title = "Add Game Sources"
        panel.message = "Choose NSP, NUP, or an extracted game folder."
        panel.canChooseFiles = true
        panel.canChooseDirectories = true
        panel.allowsMultipleSelection = true
        panel.canCreateDirectories = false
        guard panel.runModal() == .OK else { return }

        for url in panel.urls {
            guard !games.contains(where: { $0.path == url.path }) else { continue }
            let name = url.hasDirectoryPath ? url.lastPathComponent : url.deletingPathExtension().lastPathComponent
            let game = GameSource(id: UUID(), name: name, path: url.path, addedAt: Date())
            games.append(game)
        }
        games.sort { $0.name.localizedCaseInsensitiveCompare($1.name) == .orderedAscending }
        if selectedGameID == nil { selectedGameID = games.first?.id }
        saveLibrary()
    }

    func remove(_ game: GameSource) {
        games.removeAll { $0.id == game.id }
        if selectedGameID == game.id { selectedGameID = games.first?.id }
        saveLibrary()
    }

    func chooseRepository() {
        let panel = NSOpenPanel()
        panel.title = "Select NXNative Repository"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = false
        if panel.runModal() == .OK, let url = panel.url {
            repositoryPath = url.path
            UserDefaults.standard.set(repositoryPath, forKey: repositoryKey)
        }
    }

    func chooseKeys() {
        let panel = NSOpenPanel()
        panel.title = "Select prod.keys"
        panel.canChooseFiles = true
        panel.canChooseDirectories = false
        if panel.runModal() == .OK, let url = panel.url {
            keysPath = url.path
            UserDefaults.standard.set(keysPath, forKey: keysKey)
        }
    }

    func chooseOutput() {
        let panel = NSOpenPanel()
        panel.title = "Choose IPA Output Folder"
        panel.canChooseFiles = false
        panel.canChooseDirectories = true
        panel.canCreateDirectories = true
        if panel.runModal() == .OK, let url = panel.url {
            outputPath = url.path
            UserDefaults.standard.set(outputPath, forKey: outputKey)
        }
    }

    func buildSelectedGame() {
        guard !isBuilding, let game = selectedGame else { return }
        let script = URL(fileURLWithPath: repositoryPath)
            .appendingPathComponent("scripts/build_ipa_from_nsp.py")
        guard FileManager.default.fileExists(atPath: script.path) else {
            errorMessage = "Select the NXNative repository folder that contains scripts/build_ipa_from_nsp.py."
            return
        }
        guard FileManager.default.fileExists(atPath: game.path) else {
            errorMessage = "The selected game source no longer exists."
            return
        }
        guard !outputPath.isEmpty else {
            errorMessage = "Choose an output folder first."
            return
        }

        do {
            try FileManager.default.createDirectory(
                at: URL(fileURLWithPath: outputPath, isDirectory: true),
                withIntermediateDirectories: true
            )
        } catch {
            errorMessage = "Could not create the output folder: \(error.localizedDescription)"
            return
        }

        let safeName = game.name.replacingOccurrences(of: "/", with: "-")
        let ipaURL = URL(fileURLWithPath: outputPath, isDirectory: true)
            .appendingPathComponent("\(safeName)-unsigned.ipa")

        var arguments = [
            script.path, game.path,
            "--platform", "ios",
            "--progress", "json",
            "-o", ipaURL.path
        ]
        if !keysPath.isEmpty {
            arguments += ["-k", keysPath]
        }

        let task = Process()
        task.executableURL = URL(fileURLWithPath: "/usr/bin/env")
        task.arguments = ["python3"] + arguments
        task.currentDirectoryURL = URL(fileURLWithPath: repositoryPath, isDirectory: true)

        let pipe = Pipe()
        task.standardOutput = pipe
        task.standardError = pipe
        pipe.fileHandleForReading.readabilityHandler = { [weak self] handle in
            let data = handle.availableData
            if data.isEmpty {
                handle.readabilityHandler = nil
                return
            }
            guard let chunk = String(data: data, encoding: .utf8) else { return }
            DispatchQueue.main.async {
                self?.appendOutput(chunk)
            }
        }

        log = ""
        pendingProgressLine = ""
        progressFraction = nil
        progressLabel = "Starting build"
        status = "Building \(game.name)…"
        isBuilding = true
        process = task
        task.terminationHandler = { [weak self] finished in
            pipe.fileHandleForReading.readabilityHandler = nil
            DispatchQueue.main.async {
                guard let self else { return }
                self.isBuilding = false
                self.process = nil
                if finished.terminationStatus == 0 {
                    self.status = "Build completed"
                    self.appendOutput("\nBuild output: \(ipaURL.path)\n")
                } else {
                    self.status = "Build failed, exit code \(finished.terminationStatus)"
                }
            }
        }
        do {
            try task.run()
        } catch {
            pipe.fileHandleForReading.readabilityHandler = nil
            isBuilding = false
            process = nil
            status = "Could not start build"
            errorMessage = error.localizedDescription
        }
    }

    func cancelBuild() {
        process?.terminate()
        status = "Stopping build…"
    }

    private func appendOutput(_ value: String) {
        log += value
        if log.count > 100_000 {
            log = String(log.suffix(80_000))
        }

        pendingProgressLine += value
        let lines = pendingProgressLine.components(separatedBy: "\n")
        pendingProgressLine = lines.last ?? ""
        for line in lines.dropLast() {
            updateProgress(from: line)
        }
    }

    private func updateProgress(from line: String) {
        guard let data = line.data(using: .utf8),
              let event = try? JSONSerialization.jsonObject(with: data) as? [String: Any],
              let type = event["ev"] as? String else { return }

        switch type {
        case "stage":
            let name = event["name"] as? String ?? "Building"
            let index = event["k"] as? Int ?? 0
            let count = event["n"] as? Int ?? 0
            progressLabel = count > 0 ? "[\(index)/\(count)] \(name)" : name
            progressFraction = nil
        case "step":
            let name = event["name"] as? String ?? ""
            let step = event["step"] as? String ?? ""
            progressLabel = [name, step].filter { !$0.isEmpty }.joined(separator: " · ")
        case "progress":
            let name = event["name"] as? String ?? "Building"
            let step = event["step"] as? String ?? ""
            progressLabel = [name, step].filter { !$0.isEmpty }.joined(separator: " · ")
            if let fraction = event["frac"] as? NSNumber {
                progressFraction = min(max(fraction.doubleValue, 0), 1)
            }
        case "stage_end":
            let name = event["name"] as? String ?? "Stage"
            let succeeded = event["ok"] as? Bool ?? false
            progressLabel = succeeded ? "\(name) completed" : "\(name) failed"
            if succeeded { progressFraction = 1 }
        default:
            break
        }
    }

    private func saveLibrary() {
        if let data = try? JSONEncoder().encode(games) {
            UserDefaults.standard.set(data, forKey: gamesKey)
        }
    }
}

@main
struct NXNativeLauncherApp: App {
    var body: some Scene {
        WindowGroup {
            LauncherView()
                .frame(minWidth: 850, minHeight: 620)
        }
        .windowStyle(.titleBar)
    }
}

private struct LauncherView: View {
    @StateObject private var model = LauncherModel()

    var body: some View {
        VStack(spacing: 0) {
            header
            Divider()
            HStack(spacing: 0) {
                libraryPane
                    .frame(minWidth: 260, idealWidth: 290, maxWidth: 340)
                Divider()
                buildPane
            }
        }
        .background(Color(nsColor: .windowBackgroundColor))
        .alert("NXNative Launcher", isPresented: Binding(
            get: { model.errorMessage != nil },
            set: { if !$0 { model.errorMessage = nil } }
        )) {
            Button("OK", role: .cancel) { model.errorMessage = nil }
        } message: {
            Text(model.errorMessage ?? "")
        }
    }

    private var header: some View {
        HStack {
            Image(systemName: "gamecontroller.fill")
                .font(.system(size: 25, weight: .semibold))
                .foregroundStyle(.tint)
            VStack(alignment: .leading, spacing: 2) {
                Text("NXNative Launcher").font(.title2.weight(.semibold))
                Text("macOS build environment for iOS game packages")
                    .font(.caption)
                    .foregroundStyle(.secondary)
            }
            Spacer()
            Text("Unsigned IPA")
                .font(.caption.weight(.medium))
                .padding(.horizontal, 10)
                .padding(.vertical, 6)
                .background(.quaternary, in: Capsule())
        }
        .padding(18)
    }

    private var libraryPane: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("GAME SOURCES").font(.caption.weight(.semibold)).foregroundStyle(.secondary)
                Spacer()
                Button(action: model.addSources) {
                    Label("Add", systemImage: "plus")
                }
                .controlSize(.small)
                .disabled(model.isBuilding)
            }

            if model.games.isEmpty {
                VStack(spacing: 10) {
                    Image(systemName: "folder.badge.plus")
                        .font(.system(size: 30))
                        .foregroundStyle(.secondary)
                    Text("No Game Sources").font(.headline)
                    Text("Add an NSP, NUP, or extracted game folder.")
                        .font(.caption)
                        .foregroundStyle(.secondary)
                        .multilineTextAlignment(.center)
                    Button("Add Game Sources", action: model.addSources)
                        .disabled(model.isBuilding)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
            } else {
                List(selection: $model.selectedGameID) {
                    ForEach(model.games) { game in
                        VStack(alignment: .leading, spacing: 4) {
                            Text(game.name).font(.body.weight(.medium))
                            Text(game.url.lastPathComponent)
                                .font(.caption)
                                .foregroundStyle(.secondary)
                                .lineLimit(1)
                        }
                        .tag(game.id)
                        .contextMenu {
                            Button("Remove from Library", role: .destructive) {
                                model.remove(game)
                            }
                        }
                    }
                }
                .listStyle(.sidebar)
            }
            Spacer(minLength: 0)
            Text("Game files stay in their original locations.")
                .font(.caption)
                .foregroundStyle(.secondary)
        }
        .padding(14)
    }

    private var buildPane: some View {
        VStack(alignment: .leading, spacing: 16) {
            GroupBox("Build configuration") {
                VStack(alignment: .leading, spacing: 12) {
                    pathRow("NXNative repository", path: model.repositoryPath, action: model.chooseRepository)
                    pathRow("Keys file", path: model.keysPath.isEmpty ? "Optional for supported inputs" : model.keysPath, action: model.chooseKeys)
                    pathRow("IPA output folder", path: model.outputPath, action: model.chooseOutput)
                }
                .padding(.top, 4)
            }

            GroupBox("Selected game") {
                VStack(alignment: .leading, spacing: 8) {
                    if let game = model.selectedGame {
                        Text(game.name).font(.headline)
                        Text(game.path)
                            .font(.caption)
                            .foregroundStyle(.secondary)
                            .textSelection(.enabled)
                    } else {
                        Text("Select or add a game source.")
                            .foregroundStyle(.secondary)
                    }
                    HStack {
                        Button {
                            model.buildSelectedGame()
                        } label: {
                            Label("Build iOS IPA", systemImage: "hammer.fill")
                        }
                        .buttonStyle(.borderedProminent)
                        .disabled(model.selectedGame == nil || model.isBuilding)

                        if model.isBuilding {
                            Button("Cancel Build", role: .destructive, action: model.cancelBuild)
                        }
                        Spacer()
                        Text(model.status)
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                    if model.isBuilding {
                        ProgressView(value: model.progressFraction)
                        Text(model.progressLabel)
                            .font(.caption)
                            .foregroundStyle(.secondary)
                    }
                }
                .padding(.top, 4)
            }

            VStack(alignment: .leading, spacing: 6) {
                Text("BUILD OUTPUT").font(.caption.weight(.semibold)).foregroundStyle(.secondary)
                ScrollView {
                    Text(model.log.isEmpty ? "Build output and diagnostics will appear here." : model.log)
                        .font(.system(.caption, design: .monospaced))
                        .frame(maxWidth: .infinity, alignment: .leading)
                        .textSelection(.enabled)
                        .padding(10)
                }
                .background(Color(nsColor: .textBackgroundColor), in: RoundedRectangle(cornerRadius: 8))
                .overlay(RoundedRectangle(cornerRadius: 8).stroke(Color.secondary.opacity(0.2)))
            }
            .frame(maxHeight: .infinity)
        }
        .padding(16)
    }

    private func pathRow(_ title: String, path: String, action: @escaping () -> Void) -> some View {
        HStack(alignment: .center, spacing: 10) {
            VStack(alignment: .leading, spacing: 3) {
                Text(title).font(.caption.weight(.medium))
                Text(path.isEmpty ? "Not selected" : path)
                    .font(.caption)
                    .foregroundStyle(.secondary)
                    .lineLimit(1)
                    .truncationMode(.middle)
            }
            Spacer(minLength: 4)
            Button("Choose…", action: action).controlSize(.small)
        }
    }
}
