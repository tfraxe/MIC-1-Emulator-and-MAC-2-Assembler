bipush 30
bipush 40
istore a
istore b
iload a
iload b
swap
iadd
istore c
iload c
bipush 40
isub 
istore c
iload c
ifeq thiago
iload a
iload b
isub
istore c
goto l1
thiago:bipush 200
goto l2
l1:bipush 100
l2: 
