;;;; hacks.lisp
;;;;
;;;; Make sure Quicklisp is setup in your Common Lisp environment of choice
;;;;
;;;; (dolist (package '(#:str #:sdl2 #:cl-opengl #:rtg-math #:cl-autowrap #:lparallel)) (ql:quickload package))
;;;; (defpackage #:hacks
;;;;   (:use #:cl #:sdl2)
;;;;   (:import-from #:str #:split)
;;;;   (:import-from #:lparallel #:make-kernel #:end-kernel #:pdotimes)
;;;;   (:import-from #:rtg-math #:v! #:v!int #:v2! #:v2!uint #:v3! #:v3!uint8 #:v3!uint #:clamp)
;;;;   (:import-from #:autowrap #:with-calloc #:c-aref))
;;;; (load "hacks.lisp")
;;;; (hacks::run-2d-hack #'hacks::sierpinski)
;;;; (hacks::run-2d-hack #'hacks::metaballs)
;;;; (hacks::run-2d-hack #'hacks::sphere)
;;;; (hacks::run-2d-hack #'hacks::torus)
;;;; (hacks::run-2d-hack #'hacks::three-d-scene)
;;;; (hacks::run-gl-hack #'hacks::gl-shader-triangle)

(in-package #:hacks)

(defparameter +screen-size+ (cons 960 600))
(defparameter +render-size+ (cons 160 100))

(defparameter +vertices+
  (make-array 3 :initial-contents (list (cons 0 (- (cdr +render-size+) 1))
                                        (cons (floor (/ (car +render-size+) 2)) 0)
                                        (cons (- (car +render-size+) 1)
                                              (- (cdr +render-size+) 1)))))
(defvar *spoint* (cons (floor (/ (car +render-size+) 2))
                       (floor (/ (cdr +render-size+) 2))))

(defun sierpinski (renderer)
  (let* ((vertex (aref +vertices+ (random 3)))
         (vx (car vertex))
         (vy (cdr vertex))
         (sx (car *spoint*))
         (sy (cdr *spoint*))
         (dx (round (/ (- vx sx) 2)))
         (dy (round (/ (- vy sy) 2))))
    (setf *spoint* (cons (+ sx dx) (+ sy dy)))
    (with-points ((a (car *spoint*) (cdr *spoint*)))
      (set-render-draw-color renderer 0 255 0 255)
      (multiple-value-bind (points num) (points* a)
        (render-draw-points renderer points num)))))

(defparameter +n-balls+ 3)
(defparameter +min-radius+ 32)
(defparameter +max-radius+ 64)
(defparameter +large-tile+ 64)
(defvar *balls* nil)

(defun init-balls ()
  (dotimes (i +n-balls+)
    (let* ((radius
             (+ +min-radius+ (random (1+ (- +max-radius+ +min-radius+)))))
           (color (v!int (random 256) (random 256) (random 256)))
           (pos (v!int
                  (random (car +render-size+))
                  (random (cdr +render-size+))))
           (x-dir-raw (1+ (random 12)))
           (y-dir-raw (1+ (random 12)))
           (x-dir (if (> x-dir-raw 6) (- (- x-dir-raw 6)) x-dir-raw))
           (y-dir (if (> y-dir-raw 6) (- (- y-dir-raw 6)) y-dir-raw))
           (vec (v!int x-dir y-dir)))
      (push (list radius color pos vec) *balls*))))

(defun interpolate-colors (color-a color-b)
  (declare (optimize speed))
  ;; Abuse alpha as intensity
  (let* ((intensity-a (rtg-math:w color-a))
         (intensity-b (rtg-math:w color-b))
         (weak-color-4 (if (< intensity-a intensity-b) color-a color-b))
         (strong-color-4 (if (< intensity-a intensity-b) color-b color-a))
         (weak-color (v! (float (rtg-math:x weak-color-4)) (float (rtg-math:y weak-color-4)) (float (rtg-math:z weak-color-4))))
         (strong-color (v! (float (rtg-math:x strong-color-4)) (float (rtg-math:y strong-color-4)) (float (rtg-math:z strong-color-4))))
         (weak-i (rtg-math:w weak-color-4))
         (strong-i (rtg-math:w strong-color-4))
         (amount (rtg-math:lerp 0.5 0.0 (/ (- strong-i weak-i) 255.0)))
         (interpolated-color (rtg-math.vector3:lerp strong-color weak-color amount)))
    (v!int (round (rtg-math:x interpolated-color))
           (round (rtg-math:y interpolated-color))
           (round (rtg-math:z interpolated-color))
           (round (rtg-math:clamp 0.0 255.0 (float (+ weak-i strong-i)))))))

(defun color-intensity-at (vec)
  (declare (optimize speed))
  (let ((color (v!int 0 0 0 0)))
    (dolist (ball *balls*)
      (let* ((ball-x (rtg-math:x (third ball)))
             (ball-y (rtg-math:y (third ball)))
             (radius (first ball))
             (ball-color (second ball))
             (dist-square (rtg-math.vector2:distance-squared vec (v! ball-x ball-y))))
        (if (<= dist-square (expt (* 2 radius) 2))
          (let* ((dist-to-center (sqrt dist-square))
                 (dist-to-edge (- dist-to-center radius))
                 (r (rtg-math:x ball-color))
                 (g (rtg-math:y ball-color))
                 (b (rtg-math:z ball-color)))
           (if (>= 0 dist-to-edge)
             ;; We're inside a ball
             (if (= 0 (rtg-math:w color))
               (setf color (v!int r g b 255))
               (setf color (interpolate-colors color (v!int r g b 255))))
             (if (<= dist-to-edge radius)
               ;; Some contribution
               (let ((intensity (round (rtg-math:lerp 0.0 255.0 (/ (- radius dist-to-edge) radius)))))
                 (if (= 0 (rtg-math:w color))
                   (setf color (v!int r g b intensity))
                   (setf color (interpolate-colors color (v!int r g b intensity)))))))))))
    color))

(defun render-metaball-tile (renderer pos size &optional pre-ul pre-ur pre-ll pre-lr)
  (declare (optimize speed))
  (let* ((x (rtg-math:x pos))
         (y (rtg-math:y pos))
         (opposite-x (+ x size))
         (opposite-y (+ y size))
         (ul (or pre-ul (color-intensity-at (v! x y))))
         (ur (or pre-ur (color-intensity-at (v! opposite-x y))))
         (ll (or pre-ll (color-intensity-at (v! x opposite-y))))
         (lr (or pre-lr (color-intensity-at (v! opposite-x opposite-y)))))
    (if (or
          (> (rtg-math:w ul) 0)
          (> (rtg-math:w ur) 0)
          (> (rtg-math:w ll) 0)
          (> (rtg-math:w lr) 0))
      (if (> size 4)
        (let* ((new-size (/ size 2))
               (pos1 (v! (+ x new-size) y))
               (pos2 (v! x (+ y new-size)))
               (pos3 (v! (+ x new-size) (+ y new-size)))
               (pos1-ul (color-intensity-at pos1))
               (pos2-ul (color-intensity-at pos2))
               (pos3-ul (color-intensity-at pos3))
               (bottom-center (color-intensity-at (v! (+ x new-size) opposite-y)))
               (right-center (color-intensity-at (v! opposite-x (+ y new-size)))))
          (render-metaball-tile renderer pos new-size ul pos1-ul pos2-ul pos3-ul)
          (render-metaball-tile renderer pos1 new-size pos1-ul ur pos3-ul right-center)
          (render-metaball-tile renderer pos2 new-size pos2-ul pos3-ul ll bottom-center)
          (render-metaball-tile renderer pos3 new-size pos3-ul right-center bottom-center lr))
        (let* ((intensity (/ (rtg-math:w ul) 255.0))
               (rgb (rtg-math.vector3:lerp
                      (v! 0.0 0.0 0.0)
                      (v!
                        (float (rtg-math:x ul))
                        (float (rtg-math:y ul))
                        (float (rtg-math:z ul)))
                      intensity))
               (r (round (rtg-math:x rgb)))
               (g (round (rtg-math:y rgb)))
               (b (round (rtg-math:z rgb)))
               (a (round (rtg-math:w ul))))
          (if (null renderer)
            (make-rect (round x) (round y) (round (- opposite-x x)) (round (- opposite-y y)))
            (progn
              (set-render-draw-color renderer r g b a)
              (render-fill-rect renderer
                              (make-rect (round x) (round y) (round (- opposite-x x)) (round (- opposite-y y)))))))))))


(defun bounce-within (value delta min max)
  (declare (type fixnum value delta min max)
           (optimize speed))
  (let ((new (+ value delta)))
    (if (< new min)
      (values (+ min (- min new)) (- delta))
      (if (> new max)
        (values (- max (- new max)) (- delta))
        (values new delta)))))

(defun metaballs (renderer)
  (declare (optimize speed))
  (if (null *balls*)
    (init-balls))
  ;(set-render-draw-blend-mode renderer sdl2-ffi:+sdl-blendmode-blend+)
  (set-render-draw-color renderer 0 0 0 255)
  (render-clear renderer)
  (do ((x 0 (+ x +large-tile+)))
      ((>= x (car +render-size+)))
    (declare (type fixnum x))
    (do ((y 0 (+ y +large-tile+)))
        ((>= y (cdr +render-size+)))
      (declare (type fixnum y))
      (render-metaball-tile renderer (v!int x y) +large-tile+)))
  (dolist (ball *balls*)
    (let ((x (rtg-math:x (third ball)))
          (y (rtg-math:y (third ball)))
          (color (second ball)))
      (set-render-draw-color renderer (rtg-math:x color) (rtg-math:y color) (rtg-math:z color) 255)
      (render-draw-point renderer x y)))
  (dolist (ball *balls*)
    (let* ((radius (first ball))
           (x (rtg-math:x (third ball)))
           (y (rtg-math:y (third ball)))
           (dx (rtg-math:x (fourth ball)))
           (dy (rtg-math:y (fourth ball))))
      (declare (type fixnum radius x y dx dy))
      (multiple-value-bind (new-x new-dx)
        (bounce-within x dx radius (- (car +render-size+) radius))
        (setf (rtg-math:x (third ball)) new-x)
        (setf (rtg-math:x (fourth ball)) new-dx))
      (multiple-value-bind (new-y new-dy)
        (bounce-within y dy radius (- (cdr +render-size+) radius))
        (setf (rtg-math:y (third ball)) new-y)
        (setf (rtg-math:y (fourth ball)) new-dy)))))

(defmacro %rad (degrees) 
  (coerce (* pi (/ degrees 180.0)) 'single-float))

(defvar *hack-init-p* nil)
(defvar *animation-frame* 0)
(defparameter +ray-marching-steps+ 255)
(defparameter +epsilon+ 0.0001)
(defparameter +min-dist+ 0.0)
(defparameter +max-dist+ 100.0)
(defparameter +ambient-light+ (rtg-math.vector3:*s (v3! 1.0 1.0 1.0) 0.5))
(defparameter +eye+ (v3! 0.0 0.0 5.0))
(defparameter +eyeangle+ (%rad 45))
(defparameter +eyetangent+ (tan (/ +eyeangle+ 2.0)))
(declaim (type single-float +epsilon+ +min-dist+ +max-dist+ +eyeangle+)
         (type fixnum +ray-marching-steps+))

(defun ray-direction (fov size x y)
  (declare (type single-float fov)
           (type fixnum x y)
           (optimize speed))
  (let* ((frag-coordinate (v2! x y))
         (xy (rtg-math.vector2:- frag-coordinate (rtg-math.vector2:/s size 2.0)))
         (z (/ (rtg-math:y size) +eyetangent+)))
    (rtg-math.vector3:normalize (v3! (rtg-math:x xy) (rtg-math:y xy) (- z)))))

(defun distance-to-surface (sdf eye direction start end)
  (declare (type single-float start end)
           (type (function (fixnum t) single-float) sdf)
           (optimize speed))
  (let ((depth start)
        (distance (funcall sdf *animation-frame*
                               (rtg-math.vector3:+
                                 eye
                                 (rtg-math.vector3:*s direction start)))))
    (declare (type single-float depth distance))
    (do ((i 0 (1+ i)))
        ((or (>= i +ray-marching-steps+)
             (<= distance +epsilon+)
             (>= depth end)))
      (declare (type fixnum i))
      (setf distance (funcall sdf *animation-frame*
                                  (rtg-math.vector3:+
                                    eye
                                    (rtg-math.vector3:*s direction depth))))
      (if (>= distance +epsilon+)
        (setf depth (+ depth distance))))
    (if (<= distance +epsilon+)
      depth
      end)))

(declaim (inline estimate-normal))
(defun estimate-normal (sdf point)
  (declare (type (function (fixnum t) single-float) sdf)
           (type rtg-math.types:vec3 point)
           (optimize speed))
  (rtg-math.vector3:normalize
    (v3!
      (- (funcall sdf *animation-frame* (v3! (+ (rtg-math:x point) +epsilon+) (rtg-math:y point) (rtg-math:z point)))
         (funcall sdf *animation-frame* (v3! (- (rtg-math:x point) +epsilon+) (rtg-math:y point) (rtg-math:z point))))
      (- (funcall sdf *animation-frame* (v3! (rtg-math:x point) (+ (rtg-math:y point) +epsilon+) (rtg-math:z point)))
         (funcall sdf *animation-frame* (v3! (rtg-math:x point) (- (rtg-math:y point) +epsilon+) (rtg-math:z point))))
      (- (funcall sdf *animation-frame* (v3! (rtg-math:x point) (rtg-math:y point) (+ (rtg-math:z point) +epsilon+)))
         (funcall sdf *animation-frame* (v3! (rtg-math:x point) (rtg-math:y point) (- (rtg-math:z point) +epsilon+)))))))

(declaim (inline reflect))
(defun reflect (light normal)
  (rtg-math.vector3:- (rtg-math.vector3:*s (rtg-math.vector3:*s normal (rtg-math.vector3:dot normal light)) 2.0) light))

(defun phong-light-contrib (sdf diffuse-color specular-color alpha point eye light-pos light-color)
  (declare (type single-float alpha)
           (optimize speed))
  (let* ((normal (estimate-normal sdf point))
         (l (rtg-math.vector3:normalize (rtg-math.vector3:- light-pos point)))
         (v (rtg-math.vector3:normalize (rtg-math.vector3:- eye point)))
         (r (rtg-math.vector3:normalize (reflect l normal)))
         (dot-l-n (rtg-math:clamp 0.0 1.0 (rtg-math.vector3:dot l normal)))
         (dot-r-v (rtg-math.vector3:dot r v)))
    (if (< dot-l-n 0.0)
      (v3! 0.0 0.0 0.0)
      (if (< dot-r-v 0.0)
        (rtg-math.vector3:* light-color (rtg-math.vector3:*s diffuse-color dot-l-n))
        (rtg-math.vector3:*
          light-color
          (rtg-math.vector3:+ (rtg-math.vector3:*s diffuse-color dot-l-n)
                              (rtg-math.vector3:*s specular-color
                                                   (expt dot-r-v alpha))))))))

(defun shadow-depth (sdf point light-vec)
  (if (= (distance-to-surface sdf point light-vec +epsilon+ +max-dist+) +max-dist+)
    1.0
    0.0))

(defun phong-illumination (sdf point eye light-sources ambient-color diffuse-color specular-color shininess)
  (let ((point-color (rtg-math.vector3:* +ambient-light+ ambient-color)))
    (loop for light-source in light-sources do
          (setf point-color
                (rtg-math.vector3:+
                  point-color
                  (rtg-math.vector3:*s
                    (phong-light-contrib sdf diffuse-color specular-color shininess
                                         point eye (car light-source) (cdr light-source))
                    (shadow-depth sdf point (car light-source))))))
    point-color))

(defun soft-raymarch (renderer sdf
                 light-source-functions
                 diffuse-color-function specular-color-function shine-function
                 ambient-color)
  (let* ((width (car +render-size+))
         (height (cdr +render-size+))
         (screen (v! width height)))
    (with-calloc (pixels :unsigned-char (* width height 4))
      (pdotimes (x width)
        (do ((y 0 (1+ y)))
            ((>= y height))
          (let* ((px-index (* (+ (* y width) x) 4))
                 (dir (ray-direction +eyeangle+ screen x y))
                 (dist (distance-to-surface sdf +eye+ dir +min-dist+ +max-dist+)))
            (setf (c-aref pixels px-index :unsigned-char) 0)
            (setf (c-aref pixels (1+ px-index) :unsigned-char) 0)
            (setf (c-aref pixels (+ px-index 2) :unsigned-char) 0)
            (setf (c-aref pixels (+ px-index 3) :unsigned-char) 255)
            (if (<= dist (- +max-dist+ +epsilon+))
              (let* ((point
                       (rtg-math.vector3:+
                         +eye+
                         (rtg-math.vector3:*s dir dist)))
                     (color
                       (phong-illumination
                         sdf
                         point
                         +eye+
                         (mapcar
                           #'(lambda (light-fun) (funcall light-fun *animation-frame*))
                           light-source-functions)
                         ambient-color
                         (funcall diffuse-color-function *animation-frame* point)
                         (funcall specular-color-function *animation-frame* point)
                         (funcall shine-function *animation-frame* point))))
                (setf (c-aref pixels px-index :unsigned-char)
                      (round (clamp 0.0 255.0 (* 255.0 (rtg-math:x color)))))
                (setf (c-aref pixels (1+ px-index) :unsigned-char)
                      (round (clamp 0.0 255.0 (* 255.0 (rtg-math:y color)))))
                (setf (c-aref pixels (+ px-index 2) :unsigned-char)
                      (round (clamp 0.0 255.0 (* 255.0 (rtg-math:z color)))))
                (setf (c-aref pixels (+ px-index 3) :unsigned-char) 255))))))
      (let* ((surface (create-rgb-surface-from pixels width height 32 (* 4 width)
                                              :r-mask #x000000ff
                                              :g-mask #x0000ff00
                                              :b-mask #x00ff0000
                                              :a-mask #xff000000))
             (texture (create-texture-from-surface renderer surface)))
        (render-copy renderer texture)))))

(defun sphere-sdf (frame point size)
  (declare (ignore frame)
           (type single-float size))
  (the single-float (- (rtg-math.vector3:length point) size)))

(defun sphere (renderer)
  (soft-raymarch renderer
    (lambda (frame point)
      (sphere-sdf frame point 1.0))
    (list
      (lambda (frame)
        (cons
          (v3! (* 4.0 (sin (/ frame 15.0))) 2.0 (* 4.0 (cos (/ frame 15.0))))
          (v3! 0.4 0.4 0.4)))
      (lambda (frame)
        (cons
          (v3! (* 2.0 (sin (* 0.37 (/ frame 15.0)))) (* 2.0 (cos (* 0.37 (/ frame 15.0)))) 2.0)
          (v3! 0.4 0.4 0.4))))
    (lambda (frame point)
      (declare (ignore frame point))
      (the rtg-math.types:vec3 (v3! 0.7 0.2 0.2)))
    (lambda (frame point)
      (declare (ignore frame point))
      (the rtg-math.types:vec3 (v3! 1.0 1.0 1.0)))
    (lambda (frame point)
      (declare (ignore frame point))
      (the single-float 10.0))
    (v3! 0.2 0.2 0.2)))

(defun torus-sdf (frame point)
  (declare (ignore frame))
  (let ((q
          (v!
            (- (rtg-math.vector2:length (v! (rtg-math:x point) (rtg-math:z point))) 0.5)
            (rtg-math:y point))))
    (- (rtg-math.vector2:length q) 0.2)))

(defun rotate-sdf (sdf frame point)
  (funcall sdf frame (rtg-math.vector3:rotate point (v3! (sin (/ frame 15.0)) (sin (/ frame 12.0)) (cos (/ frame 15.0))))))

(defun torus (renderer)
  (soft-raymarch renderer
    (lambda (frame point)
      (rotate-sdf #'torus-sdf frame point))
    (list
      (lambda (frame)
        (cons
          (v3! (* 0.0 -10.0 0.0))
          (v3! 0.2 0.2 0.2)))
      (lambda (frame)
        (cons
          (v3! (* 4.0 (sin (/ frame 15.0))) 2.0 (* 4.0 (cos (/ frame 15.0))))
          (v3! 0.4 0.4 0.4)))
      (lambda (frame)
        (cons
          (v3! (* 2.0 (sin (* 0.37 (/ frame 15.0)))) (* 2.0 (cos (* 0.37 (/ frame 15.0)))) 2.0)
          (v3! 0.4 0.4 0.4))))
    (lambda (frame point)
      (declare (ignore frame point))
      (the rtg-math.types:vec3 (v3! 0.2 0.7 0.2)))
    (lambda (frame point)
      (declare (ignore frame point))
      (the rtg-math.types:vec3 (v3! 1.0 1.0 1.0)))
    (lambda (frame point)
      (declare (ignore frame point))
      (the single-float 10.0))
    (v3! 0.2 0.2 0.2)))

(defun plane-sdf (frame point)
  (declare (ignore frame))
  (the single-float (+ (rtg-math.vector3:dot point (v3! 0.0 -1.0 0.0)) 2.0)))

(defun three-d-scene-sdf (frame point)
  (let* ((sphere (sphere-sdf frame point 0.2))
         (torus (rotate-sdf #'torus-sdf frame point))
         (plane (plane-sdf frame point))
         (dist (min sphere torus plane)))
    (values-list (list
                   dist
                   (the rtg-math.types:vec3
                     (cond
                       ((= dist sphere) (v3! 0.7 0.2 0.2))
                       ((= dist torus) (v3! 0.2 0.7 0.2))
                       (t (v3! 0.8 0.8 0.8))))))))

(defun three-d-scene (renderer)
  (soft-raymarch renderer
    #'three-d-scene-sdf
    (list
      (lambda (frame)
        (cons
          (v3! (* 0.0 10.0 0.0))
          (v3! 0.2 0.2 0.2)))
      (lambda (frame)
        (cons
          (v3! (* 4.0 (sin (/ frame 15.0))) 2.0 (* 4.0 (cos (/ frame 15.0))))
          (v3! 0.4 0.4 0.4)))
      (lambda (frame)
        (cons
          (v3! (* 2.0 (sin (* 0.37 (/ frame 15.0)))) (* 2.0 (cos (* 0.37 (/ frame 15.0)))) 2.0)
          (v3! 0.4 0.4 0.4))))
    (lambda (frame point)
      (multiple-value-bind (dist color)
        (three-d-scene-sdf frame point)
        color))
    (lambda (frame point)
      (declare (ignore frame point))
      (the rtg-math.types:vec3 (v3! 1.0 1.0 1.0)))
    (lambda (frame point)
      (declare (ignore frame point))
      (the single-float 10.0))
    (v3! 0.2 0.2 0.2)))

(defun points-on-line (x0 y0 x1 y1)
  (declare (fixnum x0 y0 x1 y1)
           (optimize (speed 3) (safety 0)))
  (let ((x0 (if (< x0 x1) x0 x1))
        (y0 (if (< x0 x1) y0 y1))
        (x1 (if (< x0 x1) x1 x0))
        (y1 (if (< x0 x1) y1 y0)))
    (let* ((x-dist (abs (- x1 x0)))
           (y-dist (abs (- y1 y0)))
           (shallow (> x-dist y-dist))
           (n (if shallow x-dist y-dist))
           (derror (if shallow y-dist x-dist))
           (half-leg (floor n 2))
           (err 0)
           (slab-step 0)
           (point-list nil)
           (y-dir (if (< y0 y1) 1 -1)))
      (declare (fixnum x-dist y-dist n derror half-leg err slab-step))
      (dotimes (step (1+ n) t)
        (setf err (+ err derror))
        (setf point-list
              (cons (make-point (the fixnum (+ x0
                                               (if shallow
                                                   step
                                                   slab-step)))
                                (the fixnum (+ y0
                                               (* y-dir (if shallow
                                                            slab-step
                                                            step)))))
                    point-list))
        (if (> err half-leg)
            (progn
              (setf slab-step (1+ slab-step))
              (setf err (- err n)))))
      point-list)))

(defun line (renderer x0 y0 x1 y1 &optional color)
  (if color
      (set-render-draw-color renderer (rtg-math:x color) (rtg-math:y color) (rtg-math:z color) 255)
      (set-render-draw-color renderer 255 255 255 255))
  (multiple-value-bind (points num)
    (apply #'points* (points-on-line x0 y0 x1 y1))
    (render-draw-points renderer points num)))

(defun triangle (renderer v0 v1 v2 &optional color)
  (declare (optimize (speed 3) (safety 0)))
  (labels ((compare-y (v0 v1)
             (< (rtg-math:y v0) (rtg-math:y v1)))
           (compare-y-sdl (v0 v1)
             (< (point-y v0) (point-y v1))))
    (let* ((vertices (sort (list v0 v1 v2) #'compare-y))
           (bottom (car vertices))
           (mid (cadr vertices))
           (top (caddr vertices))
           (bottom-half-line (concatenate 'vector
                                          (sort (points-on-line (rtg-math:x bottom) (rtg-math:y bottom)
                                                                (rtg-math:x mid) (rtg-math:y mid))
                                                #'compare-y-sdl)))
           (top-half-line (concatenate 'vector
                                       (sort (points-on-line (rtg-math:x mid) (rtg-math:y mid)
                                                             (rtg-math:x top) (rtg-math:y top))
                                             #'compare-y-sdl)))
           (long-line (concatenate 'vector
                                   (sort (points-on-line (rtg-math:x bottom) (rtg-math:y bottom)
                                                         (rtg-math:x top) (rtg-math:y top))
                                         #'compare-y-sdl))))
      (let ((long-line-idx 0))
        (loop for line in (list bottom-half-line top-half-line)
              do (loop for i from 0 to (1- (length line))
                       do (let* ((b (aref line i))
                                 (l (aref long-line long-line-idx))
                                 (by (point-y b))
                                 (ly (point-y l))) 
                            (if (> by ly)
                                (loop while (> by ly)
                                      do (progn
                                           (setf long-line-idx (1+ long-line-idx))
                                           (setf l (aref long-line long-line-idx))
                                           (setf ly (point-y l)))))
                            (if (= by ly )
                                (line renderer (point-x b) by
                                      (point-x l) ly
                                      color)))))))))

(defun gl-ff-triangle ()
  (gl:clear-color 0.0 0.0 0.0 1.0)
  (gl:clear :color-buffer)
  (gl:matrix-mode :modelview)
  (gl:rotate 1.0 1.0 1.0 1.0)
  (gl:begin :triangles)
  (gl:color 1.0 0.0 0.0 1.0)
  (gl:vertex 0.0 0.5)
  (gl:color 0.0 1.0 0.0 1.0)
  (gl:vertex 0.5 -0.5)
  (gl:color 0.0 0.0 1.0 1.0)
  (gl:vertex -0.5 -0.5)
  (gl:end)
  (gl:flush))

(defparameter +tri-vertex-shader-glsl+
  "#version 130

  uniform int frame;
  uniform mat4 perspectiveMatrix;
  in vec3 vertexPosition_modelspace;
  in vec3 vertexColor;
  out vec3 fragmentColor;

  mat4 rotationMatrix(vec3 axis, float angle) {
    axis = normalize(axis);
    float s = sin(angle);
    float c = cos(angle);
    float oc = 1.0 - c;

    return mat4(         oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s, oc * axis.z * axis.x + axis.y * s, 0.0,
                oc * axis.x * axis.y + axis.z * s,          oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s, 0.0,
                oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s,          oc * axis.z * axis.z + c, 0.0,
                                              0.0,                               0.0,                               0.0, 1.0);
  }

  void main() {
    vec4 vertex = vec4(vertexPosition_modelspace, 1.0);
    float angle = radians(mod(frame, 360.0));
    vertex = vertex * rotationMatrix(vec3(1.0, 1.0, 1.0), angle);
    gl_Position = vertex * perspectiveMatrix;
    fragmentColor = vertexColor;
  }")

(defparameter +tri-fragment-shader-glsl+
  "#version 130

  in vec3 fragmentColor;
  out vec3 color;

  void main() {
    color = fragmentColor;
  }")

(defparameter +perspective-matrix+ (rtg-math.projection:perspective 960.0 600.0 0.1 100.0 45.0))
(defvar *tri-vertex-buffer*)
(defvar *tri-color-buffer*)
(defvar *tri-vertex-shader*)
(defvar *tri-fragment-shader*)
(defvar *tri-shader-program*)
(defvar *frame-uniform-location*)
(defvar *perspective-uniform-location*)

(defun gl-shader-triangle ()
  (if (null *hack-init-p*)
    (progn
      (setf *hack-init-p* t)
      (let ((varr (gl:alloc-gl-array :float 9))
            (carr (gl:alloc-gl-array :float 9))
            (verts #(-0.5 -0.5 0.0
                      0.5 -0.5 0.0
                      0.0  0.5 0.0))
            (colors #(1.0 0.0 0.0
                      0.0 1.0 0.0
                      0.0 0.0 1.0)))
        (dotimes (i (length verts))
          (setf (gl:glaref varr i) (aref verts i))
          (setf (gl:glaref carr i) (aref colors i)))
        ; setup vertex array
        (gl:bind-vertex-array (gl:gen-vertex-array))
        (setf *tri-vertex-buffer* (gl:gen-buffer))
        (gl:bind-buffer :array-buffer *tri-vertex-buffer*)
        (gl:buffer-data :array-buffer :static-draw varr)
        (gl:free-gl-array varr)
        ; setup color array
        (gl:bind-vertex-array (gl:gen-vertex-array))
        (setf *tri-color-buffer* (gl:gen-buffer))
        (gl:bind-buffer :array-buffer *tri-color-buffer*)
        (gl:buffer-data :array-buffer :static-draw carr)
        (gl:free-gl-array carr))
      (let ((vs (gl:create-shader :vertex-shader))
            (fs (gl:create-shader :fragment-shader)))
        (setf *tri-vertex-shader* vs)
        (setf *tri-fragment-shader* fs)
        (gl:shader-source vs +tri-vertex-shader-glsl+)
        (gl:compile-shader vs)
        (gl:shader-source fs +tri-fragment-shader-glsl+)
        (gl:compile-shader fs)
        (setf *tri-shader-program* (gl:create-program))
        (gl:attach-shader *tri-shader-program* vs)
        (gl:attach-shader *tri-shader-program* fs)
        (gl:bind-attrib-location *tri-shader-program* 0 "vertexPosition_modelspace")
        (gl:bind-attrib-location *tri-shader-program* 1 "vertexColor")
        (gl:link-program *tri-shader-program*)
        (gl:validate-program *tri-shader-program*)
        (print (gl:get-shader-info-log vs))
        (print (gl:get-shader-info-log fs))
        (setf *frame-uniform-location* (gl:get-uniform-location *tri-shader-program* "frame"))
        (setf *perspective-uniform-location* (gl:get-uniform-location *tri-shader-program* "perspectiveMatrix"))
        (gl:use-program *tri-shader-program*))))

  (gl:uniformi *frame-uniform-location* *animation-frame*)
  (gl:uniform-matrix-4fv *perspective-uniform-location* +perspective-matrix+)
  (gl:enable-vertex-attrib-array 0)
  (gl:bind-buffer :array-buffer *tri-vertex-buffer*)
  (gl:vertex-attrib-pointer 0 3 :float 0 0 0)
  (gl:enable-vertex-attrib-array 1)
  (gl:bind-buffer :array-buffer *tri-color-buffer*)
  (gl:vertex-attrib-pointer 1 3 :float 0 0 0)
  (gl:draw-arrays :triangles 0 3)
  (gl:disable-vertex-attrib-array 0)
  (gl:disable-vertex-attrib-array 1))

(defun run-2d-hack (hack)
  "The whole enchilada (renderer)"
  (setf *hack-init-p* nil)
  (setf *animation-frame* 0)
  (with-init (:everything)
    (format t "Using SDL Library Version: ~D.~D.~D~%"
            sdl2-ffi:+sdl-major-version+
            sdl2-ffi:+sdl-minor-version+
            sdl2-ffi:+sdl-patchlevel+)
    (finish-output)

    (with-window (win :w (car +screen-size+) :h (cdr +screen-size+) :flags '(:shown))
      (with-renderer (renderer win :flags '(:accelerated :presentvsync))
        (let ((texture (create-texture renderer :rgba8888 :target (car +render-size+) (cdr +render-size+))))
          (setf lparallel:*kernel* (make-kernel 4))
          (with-event-loop (:method :poll)
            (:keyup
              (:keysym keysym)
              (when (scancode= (scancode-value keysym) :scancode-escape)
                (push-event :quit)))
            (:idle
              ()
              (set-render-draw-color renderer 0 0 0 255)
              (render-clear renderer)
              (set-render-target renderer texture)
              (funcall hack renderer)
              (set-render-target renderer nil)
              (render-copy renderer texture)
              (render-present renderer)
              (setf *animation-frame* (1+ *animation-frame*))
              (delay 1))
            (:quit () t)))
        (end-kernel)))))

(defun run-gl-hack (hack)
  "The whole enchilada (OpenGL)"
  (setf *hack-init-p* nil)
  (setf *animation-frame* 0)
  (with-init (:everything)
    (format t "Using SDL Library Version: ~D.~D.~D~%"
            sdl2-ffi:+sdl-major-version+
            sdl2-ffi:+sdl-minor-version+
            sdl2-ffi:+sdl-patchlevel+)
    (finish-output)

    (with-window (win :w (car +screen-size+) :h (cdr +screen-size+) :flags '(:shown :opengl))
      (with-gl-context (gl-context win)
        (setf lparallel:*kernel* (make-kernel 4))
        (with-event-loop (:method :poll)
          (:keyup
            (:keysym keysym)
            (when (scancode= (scancode-value keysym) :scancode-escape)
              (push-event :quit)))
          (:idle
            ()
            (gl:clear-color 0.0 0.0 0.0 1.0)
            (gl:clear :color-buffer)
            (funcall hack)
            (sdl2:gl-swap-window win)
            (setf *animation-frame* (1+ *animation-frame*))
            (delay 1))
          (:quit () t)))
      (end-kernel))))

