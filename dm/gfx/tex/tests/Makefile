# Texture sub-module unit tests (seconds): make run
CFLAGS = -O2 -g -Wall -Wextra
SAN ?=
O ?=
CLANG = clang $(SAN)
$(O)t_tex: t_tex.c ../nvn_deswizzle.c ../nvn_deswizzle.h ../../nvn/nvn_tex.c ../../nvn/nvn_tex.h
	$(CLANG) $(CFLAGS) -o $@ t_tex.c ../nvn_deswizzle.c ../../nvn/nvn_tex.c -lm
$(O)t_fmt_mtl: t_fmt_mtl.m ../nvn_fmt_mtl.c ../nvn_fmt_mtl.h
	$(CLANG) $(CFLAGS) -fobjc-arc -o $@ t_fmt_mtl.m ../nvn_fmt_mtl.c -framework Metal -framework Foundation
$(O)t_bc_decode: t_bc_decode.c ../nvn_bc_decode.c ../nvn_bc_decode.h ../bcdec.h
	$(CLANG) $(CFLAGS) -o $@ t_bc_decode.c ../nvn_bc_decode.c
run: t_tex t_fmt_mtl t_bc_decode
	./t_fmt_mtl && ./t_tex && ./t_bc_decode
.PHONY: run
include ../../../sanitizers/sanitizers.mk
.PHONY: asan
asan:
	@$(call san_fresh,asan,$(ASAN_FLAGS))
	$(MAKE) O=asan/ SAN="$(ASAN_FLAGS)" asan/t_tex asan/t_bc_decode
	$(ASAN_ENV) ./asan/t_tex
	$(ASAN_ENV) ./asan/t_bc_decode
include ../../../sanitizers/coverage.mk
.PHONY: coverage
coverage:
	@$(call san_fresh,cov,$(COV_FLAGS))
	rm -rf cov/prof
	$(MAKE) O=cov/ SAN="$(COV_FLAGS)" cov/t_tex cov/t_bc_decode
	-LLVM_PROFILE_FILE="$(call cov_profile,cov)" ./cov/t_tex
	-LLVM_PROFILE_FILE="$(call cov_profile,cov)" ./cov/t_bc_decode
	@$(call cov_export,cov)
