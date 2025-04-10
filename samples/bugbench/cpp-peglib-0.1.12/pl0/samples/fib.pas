VAR i, x, r;

PROCEDURE fib;
VAR xx, r1, r2;
BEGIN
  xx := x;
  IF xx = 0 THEN r := 1;
  IF xx = 1 THEN r := 1;
  IF xx >= 2 THEN BEGIN
    x := xx - 2;
    CALL fib;
    r1 := r;

    x := xx - 1;
    CALL fib;
    r2 := r;
    r := r1 + r2;
  END
END;

BEGIN
  i := 0;
  WHILE i < 25 DO BEGIN
    x := i;
    CALL fib;
    write i;
    write r;
    i := i + 1;
  END
END.
