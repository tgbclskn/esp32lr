cbmc --function app_main \
--drop-unused-functions \
--memory-leak-check \
--memory-cleanup-check \
--float-div-by-zero-check \
--unsigned-overflow-check \
--pointer-overflow-check \
--conversion-check \
--float-overflow-check \
--nan-check \
--enum-range-check \
--retain-trivial-checks \
--verbosity 8 \
--validate-goto-model \
--validate-ssa-equation \
--malloc-may-fail \
--trace \
--unwinding-assertions \
main.c > verif.log
