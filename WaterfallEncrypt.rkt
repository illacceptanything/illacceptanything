#lang racket

(require openssl/sha1)
(provide waterfall)

;HELPERS
;====================================
;Takes the inverse of the list
(define (inverse-list key-list)
  (map
    (lambda (x)
     (* -1 x))
   key-list))

;Replace password->key-list
(define (string->integer-list in)
  (map
   (lambda (x)
     (char->integer x))
   (string->list in)))

;Takes a list of strings and appends them together
(define (list-strings->string in)
  (let loop ((out "")
             (curr (reverse in)))
    (if (empty? curr)
        out
        (loop (string-append (car curr) out)
              (cdr curr)))
    ))

;Split the input list of characters into strings of len length
(define (split-list input len)
  (let loop ((out '() )
             (curr input))
    (if (> len (length curr))
        ;Remove potential empty string in results
        (let ((res (reverse (cons (list->string curr)
                                  out))))
          (if (string=? (last res) "")
              (take res (- (length res) 1))
              res))
        ;Add len elements to output
        (loop (cons (list->string (take curr len))
                    out)
              (list-tail curr len))
        )))

;ENCRYPTION FUNCS
;====================================
;Encrypts input with the key. Simple Viegenere Cipher
(define (cipher input key)
  (let loop ((in (string->integer-list input))
             (k key))
    ;Buffer key to length of input
    (if (> (length in) (length k))
        (loop in
              (flatten (cons k k)))
        ;Substitution cipher, no negative values
        (list->string
         (map
          (lambda (x y)
            (if (< 0 (+ x y))
                (integer->char (+ x y))
                (integer->char (+ x 0))))
          in (take k (length in)))
         ))
    ))

;Encrypts a list of characters broken into sublists
(define (waterfall-encrypt input key)
  (let loop ((out '() )
             (curr input))
    
    (if (= 1 (length curr))
        ;Add on the first element, encrypted with the key
        (cons (cipher (car input) key)
              (reverse out))
        ;Encrypt the next element with the current element as its key
        (loop (cons (cipher (car (cdr curr))
                            (string->integer-list (car curr)))
                    out)
              (cdr curr))
        )))

;Decrypts a waterfall encrypted list of characters
(define (waterfall-decrypt input key)
  (let loop ((out (list (cipher (car input) key)))
             (curr input))
    
    (if (= 1 (length curr))
        (reverse out)
        ;Decrypt the next element with the current, already decrypted element
        (loop (cons (cipher (car (cdr curr))
                            (inverse-list (string->integer-list (car out))))
                    out)
              (cdr curr))
        )))

;MAIN
;====================================
;Encrypts or decrypts strings
(define (waterfall input key mode)
  ;Prepare input: hash password, in -> list
  (let ((key-list (string->integer-list
                   (sha1 (open-input-bytes
                          (string->bytes/locale key)))))
        (in (string->list input)))
    ;Get length for spliting, and split the input
    (letrec ((len (length key-list))           
             (lists (split-list in len)))
      
      (if (equal? mode #t)
          ;Encrypt
          (list-strings->string
           (waterfall-encrypt lists key-list))
          ;Decrypt
          (list-strings->string
           (waterfall-decrypt lists (inverse-list key-list))))
      )))

(define key "Stupendous")
(define message "In 1863 Friedrich Kasiski was the first to publish a successful general attack on the Vigenère cipher.\
 Earlier attacks relied on knowledge of the plaintext, or use of a recognizable word as a key. Kasiski's method had no such\
 dependencies. Kasiski was the first to publish an account of the attack, but it is clear that there were others who were aware\
 of it. In 1854, Charles Babbage was goaded into breaking the Vigenère cipher when John Hall Brock Thwaites submitted a new \
cipher to the Journal of the Society of the Arts. When Babbage showed that Thwaites' cipher was essentially just another \
recreation of the Vigenère cipher, Thwaites challenged Babbage to break his cipher encoded twice, with keys of different length.\
 Babbage succeeded in decrypting a sample, which turned out to be the poem The Vision of Sin, by Alfred Tennyson, encrypted \
according to the keyword Emily, the first name of Tennyson's wife. Babbage never explained the method he used. Studies of \
Babbage's notes reveal that he had used the method later published by Kasiski, and suggest that he had been using the method as early as 1846.")

(define (test)
  ;Encrypt and save
  (define encrypted (waterfall message key #t))
  (display encrypted)(newline)(newline)
  ;Decrypt
  (display (waterfall encrypted key #f))
  )

(test)

