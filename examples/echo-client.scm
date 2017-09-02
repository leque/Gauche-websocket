(use libwebsockets)
(use gauche.uvector)

(let ((client (make-websocket-client
               "ws://echo.example.org/"
               :on-open (lambda (c)
                          (print "connection opened: " c))
               :on-close (lambda (c)
                           (print "connection closed: " c))
               :on-error (lambda (c)
                           (print "connection error: " c))
               :on-message (lambda (c in)
                             (print "some message arrived: "
                                    (u8vector->string in)
                                    c))
               :on-writable (lambda (c)
                              (print "writable: " c)
                              (websocket-client-write c "foobar")
                              (sys-sleep 3)))))
  (while #t
    (websocket-client-service client 50)))
