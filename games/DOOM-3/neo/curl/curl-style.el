;;;; Emacs Lisp help for writing curl code. ;;;;
;;;; $Id: curl-style.el,v 1.7 2004/03/09 22:55:47 bagder Exp $

;;; The curl hacker's C conventions.

;;; After loading this file and added the mode-hook you can in C
;;; files, put something like this to use the curl style
;;; automatically:
;;
;;   /* -----------------------------------------------------------------
;;    * local variables:
;;    * eval: (set c-file-style "curl")
;;    * end:
;;    */
;;

(defconst curl-c-style
  '((c-basic-offset . 2)
    (c-comment-only-line-offset . 0)
    (c-hanging-braces-alist     . ((substatement-open before after)))
    (c-offsets-alist . ((topmost-intro        . 0)
			(topmost-intro-cont   . 0)
			(substatement         . +)
			(substatement-open    . 0)
			(statement-case-intro . +)
			(statement-case-open  . 0)
			(case-label           . 0)
			))
    )
  "Curl C Programming Style")

;; Customizations for all of c-mode, c++-mode, and objc-mode
(defun curl-c-mode-common-hook ()
  "Curl C mode hook"
  ;; add curl style and set it for the current buffer
  (c-add-style "curl" curl-c-style t)
  (setq tab-width 8
	indent-tabs-mode nil		; Use spaces. Not tabs.
	comment-column 40
	c-font-lock-extra-types (append '("bool" "CURL" "CURLcode" "ssize_t" "size_t" "socklen_t" "fd_set" "time_t" "curl_off_t" "curl_socket_t"))
	)
  ;; keybindings for C, C++, and Objective-C.  We can put these in
  ;; c-mode-base-map because of inheritance ...
  (define-key c-mode-base-map "\M-q" 'c-fill-paragraph)
  (setq c-recognize-knr-p nil)
  )

;; Set this is in your .emacs if you want to use the c-mode-hook as
;; defined here right out of the box.
; (add-hook 'c-mode-common-hook 'curl-c-mode-common-hook)
