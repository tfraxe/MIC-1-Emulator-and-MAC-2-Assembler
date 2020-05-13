bipush 5
bipush 10
iand
ifeq l1
istore h
l1:istore l
bipush 40
bipush 30
isub
goto l2
istore j
bipush 20
istore h
l2:istore h
bipush 30
istore j



