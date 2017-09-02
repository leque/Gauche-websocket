;;;
;;; Test libwebsockets
;;;

(use gauche.test)

(test-start "libwebsockets")
(use libwebsockets)
(test-module 'libwebsockets)

;; The following is a dummy test code.
;; Replace it for your tests.
(test* "test-websocket" "websocket is working"
       (test-websocket))

;; If you don't want `gosh' to exit with nonzero status even if
;; the test fails, pass #f to :exit-on-failure.
(test-end :exit-on-failure #t)




