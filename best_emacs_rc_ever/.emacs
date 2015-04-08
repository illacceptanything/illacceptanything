(when (load "flymake" t)
  (defun flymake-pyflakes-init ()
    (let* ((temp-file (flymake-init-create-temp-buffer-copy
                       'flymake-create-temp-inplace))
           (local-file (file-relative-name
                        temp-file
                        (file-name-directory buffer-file-name))))
      (list "pyboth" (list local-file))))
  (add-to-list 'flymake-allowed-file-name-masks
               '("\\.py\\'" flymake-pyflakes-init)))

(add-hook 'find-file-hook 'flymake-find-file-hook)

(add-hook 'before-save-hook 'delete-trailing-whitespace)

(defun flymake-display-warning (warning)
  "Display a warning to the user, using minibuffer"
  (message warning))

(require 'color-theme)
(color-theme-initialize)
(color-theme-billw)

(column-number-mode)

(setq-default indent-tabs-mode nil);
(setq tab-width 4);
(customize-variable (quote tab-stop-list))


(custom-set-variables
  ;; custom-set-variables was added by Custom.
  ;; If you edit it by hand, you could mess it up, so be careful.
  ;; Your init file should contain only one such instance.
  ;; If there is more than one, they won't work right.
 '(menu-bar-mode nil nil (menu-bar))
 '(safe-local-variable-values (quote ((encoding . utf-8))))
 '(tool-bar-mode nil nil (tool-bar)))

(custom-set-faces
  ;; custom-set-faces was added by Custom.
  ;; If you edit it by hand, you could mess it up, so be careful.
  ;; Your init file should contain only one such instance.
  ;; If there is more than one, they won't work right.
 )

(setq-default tab-width 4) (setq-default indent-tabs-mode nil)

(desktop-save-mode 1)
(put 'scroll-left 'disabled nil)
(delete-selection-mode 1)
(auto-fill-mode -1)
(remove-hook 'text-mode-hook #'turn-on-auto-fill)

;;Ido
 (require 'ido) 
 (ido-mode t) 
 (setq ido-enable-flex-matching t) ;; enable fuzzy matching 
 ;; idomenu 
 (autoload 'idomenu "idomenu" nil t)
