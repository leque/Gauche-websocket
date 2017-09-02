;;;
;;; Test libwebsockets
;;;

(use gauche.test)

(test-start "libwebsockets")
(use libwebsockets)
(test-module 'libwebsockets)

;; If you don't want `gosh' to exit with nonzero status even if
;; the test fails, pass #f to :exit-on-failure.
(test-end :exit-on-failure #t)
