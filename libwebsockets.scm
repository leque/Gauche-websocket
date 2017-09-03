;;;
;;; websocket
;;;

(define-module libwebsockets
  (export make-websocket-client
          websocket-client-service
          websocket-client-write
          )
  (use rfc.uri)
  )
(select-module libwebsockets)

;; Loads extension
(dynamic-load "websocket")

(define (%make-websocket-callback
         :key on-open on-close on-message on-writable on-error)
  (lambda (client in reason)
    (cond ((= reason LWS_CALLBACK_CLIENT_ESTABLISHED)
           (on-open client))
          ((= reason LWS_CALLBACK_CLOSED)
           (on-close client))
          ((= reason LWS_CALLBACK_CLIENT_RECEIVE)
           (on-message client in))
          ((= reason LWS_CALLBACK_CLIENT_WRITEABLE)
           (on-writable client))
          ((= reason LWS_CALLBACK_CLIENT_CONNECTION_ERROR)
           (on-error client))
          (else
           (warn "unhandled callback:" reason)))
    0))

(define (make-websocket-client url
                               :key
                               (headers '())
                               on-open
                               on-close
                               on-message
                               on-writable
                               on-error)
  (receive (scheme _usr-info host port path query fragment)
      (uri-parse url)
    (let ((secure (cond
                   ((equal? scheme "wss") #t)
                   ((equal? scheme "ws") #f)
                   (else (error "uri scheme should be ws or wss:" url))))
          (callback (%make-websocket-callback
                     :on-open on-open
                     :on-close on-close
                     :on-message on-message
                     :on-writable on-writable
                     :on-error on-error)))
      (rlet1 c (%make-websocket-client host
                                       (or port (if secure 443 80))
                                       (if secure 1 0)
                                       path
                                       host
                                       ""
                                       callback
                                       headers)
        (%websocket-client-connect c)))))
