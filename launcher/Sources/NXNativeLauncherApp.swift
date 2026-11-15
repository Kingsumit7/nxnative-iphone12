import SwiftUI
import Combine
import UniformTypeIdentifiers

private enum LauncherConfiguration {
    static let libraryDirectoryName = "Games"
    static let libraryFileName = "library.json"
}

private struct LibraryGame: Identifiable, Codable, Hashable {
    var id: UUID
    var title: String
    var fileName: String
    var relativePath: String
    var importedAt: Date

    var fileURL: URL {
        Self.libraryDirectory.appendingPathComponent(relativePath)
    }

    static var libraryDirectory: URL {
        let base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0]
        return base.appendingPathComponent(LauncherConfiguration.libraryDirectoryName, isDirectory: true)
    }

    static var indexURL: URL {
        libraryDirectory.appendingPathComponent(LauncherConfiguration.libraryFileName)
    }
}

@MainActor
private final class GameLibrary: ObservableObject {
    @Published private(set) var games: [LibraryGame] = []
    @Published var errorMessage: String?

    init() {
        load()
    }

    func importFiles(_ urls: [URL]) {
        do {
            try FileManager.default.createDirectory(
                at: LibraryGame.libraryDirectory,
                withIntermediateDirectories: true
            )

            for sourceURL in urls {
                let hasAccess = sourceURL.startAccessingSecurityScopedResource()
                defer {
                    if hasAccess {
                        sourceURL.stopAccessingSecurityScopedResource()
                    }
                }

                let safeName = sourceURL.lastPathComponent
                let destinationName = "\(UUID().uuidString)-\(safeName)"
                let destinationURL = LibraryGame.libraryDirectory.appendingPathComponent(destinationName)

                try FileManager.default.copyItem(at: sourceURL, to: destinationURL)

                let game = LibraryGame(
                    id: UUID(),
                    title: sourceURL.deletingPathExtension().lastPathComponent,
                    fileName: safeName,
                    relativePath: destinationName,
                    importedAt: Date()
                )
                games.append(game)
            }

            try save()
            errorMessage = nil
        } catch {
            errorMessage = "Import failed: \(error.localizedDescription)"
            load()
        }
    }

    func remove(_ game: LibraryGame) {
        do {
            try FileManager.default.removeItem(at: game.fileURL)
            games.removeAll { $0.id == game.id }
            try save()
        } catch {
            errorMessage = "Could not remove the selected game: \(error.localizedDescription)"
        }
    }

    private func load() {
        do {
            guard FileManager.default.fileExists(atPath: LibraryGame.indexURL.path) else {
                games = []
                return
            }
            let data = try Data(contentsOf: LibraryGame.indexURL)
            games = try JSONDecoder().decode([LibraryGame].self, from: data)
                .filter { FileManager.default.fileExists(atPath: $0.fileURL.path) }
                .sorted { $0.title.localizedCaseInsensitiveCompare($1.title) == .orderedAscending }
        } catch {
            games = []
            errorMessage = "The game library could not be read: \(error.localizedDescription)"
        }
    }

    private func save() throws {
        let data = try JSONEncoder().encode(games)
        try data.write(to: LibraryGame.indexURL, options: .atomic)
    }
}

@main
struct NXNativeLauncherApp: App {
    var body: some Scene {
        WindowGroup {
            LibraryView()
        }
    }
}

private struct LibraryView: View {
    @StateObject private var library = GameLibrary()
    @State private var showingImporter = false
    @State private var selectedGame: LibraryGame?

    private let supportedTypes: [UTType] = [
        UTType(filenameExtension: "nsp") ?? .data,
        UTType(filenameExtension: "xci") ?? .data,
        UTType(filenameExtension: "nsz") ?? .data,
        UTType(filenameExtension: "nup") ?? .data,
        UTType(filenameExtension: "xcz") ?? .data
    ]

    var body: some View {
        NavigationStack {
            Group {
                if library.games.isEmpty {
                    ContentUnavailableView {
                        Label("Your Library Is Empty", systemImage: "gamecontroller")
                    } description: {
                        Text("Import a game file to add it to your library. Game launching is not available in this prototype.")
                    } actions: {
                        Button {
                            showingImporter = true
                        } label: {
                            Label("Import Game", systemImage: "plus")
                        }
                        .buttonStyle(.borderedProminent)
                    }
                } else {
                    List {
                        ForEach(library.games) { game in
                            Button {
                                selectedGame = game
                            } label: {
                                HStack(spacing: 12) {
                                    Image(systemName: "gamecontroller.fill")
                                        .font(.title2)
                                        .foregroundStyle(.tint)
                                        .frame(width: 44, height: 44)
                                        .background(.quaternary, in: RoundedRectangle(cornerRadius: 10))
                                    VStack(alignment: .leading, spacing: 4) {
                                        Text(game.title)
                                            .font(.headline)
                                            .foregroundStyle(.primary)
                                        Text(game.fileName)
                                            .font(.caption)
                                            .foregroundStyle(.secondary)
                                            .lineLimit(1)
                                    }
                                    Spacer()
                                    Image(systemName: "chevron.right")
                                        .font(.caption.weight(.semibold))
                                        .foregroundStyle(.tertiary)
                                }
                                .padding(.vertical, 4)
                            }
                            .contextMenu {
                                Button("Remove from Library", role: .destructive) {
                                    library.remove(game)
                                }
                            }
                        }
                    }
                    .listStyle(.insetGrouped)
                }
            }
            .navigationTitle("NXNative")
            .toolbar {
                ToolbarItem(placement: .topBarTrailing) {
                    Button {
                        showingImporter = true
                    } label: {
                        Label("Import", systemImage: "plus")
                    }
                }
            }
            .fileImporter(
                isPresented: $showingImporter,
                allowedContentTypes: supportedTypes,
                allowsMultipleSelection: true
            ) { result in
                switch result {
                case .success(let urls):
                    library.importFiles(urls)
                case .failure(let error):
                    library.errorMessage = error.localizedDescription
                }
            }
            .alert("Library", isPresented: Binding(
                get: { library.errorMessage != nil },
                set: { if !$0 { library.errorMessage = nil } }
            )) {
                Button("OK", role: .cancel) { library.errorMessage = nil }
            } message: {
                Text(library.errorMessage ?? "")
            }
            .sheet(item: $selectedGame) { game in
                GameDetailsView(game: game)
            }
        }
        .tint(.indigo)
    }
}

private struct GameDetailsView: View {
    let game: LibraryGame
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        NavigationStack {
            Form {
                Section("Game") {
                    LabeledContent("Title", value: game.title)
                    LabeledContent("File", value: game.fileName)
                    LabeledContent("Imported", value: game.importedAt.formatted(date: .abbreviated, time: .shortened))
                }

                Section {
                    Label("Runtime integration pending", systemImage: "info.circle")
                    Text("This prototype manages imported files only. It does not decrypt, convert, or launch games.")
                        .font(.footnote)
                        .foregroundStyle(.secondary)
                }
            }
            .navigationTitle("Game Details")
            .navigationBarTitleDisplayMode(.inline)
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("Done") { dismiss() }
                }
            }
        }
    }
}
