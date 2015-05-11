¢ algol ¢

main:(
  FOR i TO 100 DO
    printf(($gl$,
      IF i %* 15 = 0 THEN
        "fizzbuzz"
      ELIF i %* 3 = 0 THEN
        "fizz"
      ELIF i %* 5 = 0 THEN
        "buzz"
      ELSE
        i
      FI
    ))
  OD
)

¢ @lizmrush ¢
