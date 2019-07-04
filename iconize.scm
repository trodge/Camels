(define (script-fu-iconize img drawable size threshold)
  (gimp-image-undo-group-start img)
  (gimp-context-push)
  (let ((w (car (gimp-image-width img)))
        (h (car (gimp-image-height img))))
    (gimp-context-set-background (car (gimp-image-pick-color img drawable 0 0 0 1 20)))
    (gimp-image-resize img (+ w 10) (+ h 10) 5 5)
    (gimp-layer-resize-to-image-size drawable)
    (gimp-context-set-sample-threshold-int threshold)
    (gimp-image-select-contiguous-color
      img
      CHANNEL-OP-ADD
      drawable
      0
      0)
    (gimp-layer-add-alpha drawable)
    (gimp-drawable-edit-clear drawable)
    (plug-in-autocrop RUN-INTERACTIVE img drawable)
    (set! w (car (gimp-image-width img)))
    (set! h (car (gimp-image-height img)))
    (if (> w h) (gimp-image-resize img w w 0 (/ (- w h) 2))
                (gimp-image-resize img h h (/ (- h w) 2) 0)))
    (gimp-layer-resize-to-image-size drawable)
    (gimp-image-scale img size size)
  (gimp-context-pop)
  (gimp-image-undo-group-end img)
  (gimp-displays-flush))

(script-fu-register
  "script-fu-iconize"
  "<Image>/Filters/Iconize"
  "Description of plugin"
  "Tom Rodgers"
  "Tom Rodgers"
  "July 2019"
  "*"
  SF-IMAGE
  "Image"
  0
  SF-DRAWABLE
  "Drawable"
  0
  SF-ADJUSTMENT
  "Size"
  '(51 1 100 1 10 1 1)
  SF-ADJUSTMENT
  "Threshold"
  '(31 1 100 1 10 1 1))
