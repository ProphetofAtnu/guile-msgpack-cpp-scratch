(use-modules (ice-9 binary-ports))

(load-extension "libguile-mpack.dylib" "guile_msgpack_module_init")

(define bin (call-with-input-file "demo.bin"
                                  get-bytevector-all))

(add-to-load-path "./guile-msgpack/")

(use-modules (msgpack))
