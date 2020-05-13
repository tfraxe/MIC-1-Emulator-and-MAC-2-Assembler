# coding: utf-8

import re, sys

def isNumber(number):
    numberRegex = re.compile(r'\b[0-9]+\b')
    matchedNumber = numberRegex.search(number)
    if (matchedNumber != None and len(number) == len(matchedNumber.group())):
        return True
    else:
        return False


assemblyRegex = re.compile(r'''(
([a-zA-Z0-9]+:)? #Nome do label, aceita letras e numeros: opcional
([a-z_]+)?        #Nome da instrução, aceita letras e _: obrigatório    
\s               #Após o nome da instrução, há espaço ou quebra de linha
([(a-z)?0-9]+)?  #Nome do operando, aceita letras e número: opcional
(\s[0-9]+)?      #Segundo operando, se houver
)''', re.VERBOSE) 

assembly = [] #Esta lista vai guardar cada linha do programa a ser montado
programa = ''
if len(sys.argv) == 2:
    programa = sys.argv[1]
    print(programa)
else:
    print('Erro! Nenhum arquivo encontrado')
    sys.exit()
with open(programa) as prog:
    assembly = prog.readlines() #Lê cada linha do programa e adiciona na lista assembly
   
memory = bytearray([]) #inicializando memory como um array de bytes

varDict = {} #inicializa dicionário das variáveis. As chaves serão os nomes das variáveis 
             #e os valores serão um inteiro que representa a ordem 
             #em que a variável aparece no código
             #Ex.: Se i é a primeira variável a aparecer e j é a segunda, então
             #{'i': 0, 'j': 1}

numVars = 0 #No início da montagem, o número de variáveis é igual a 0. 

byteInstructionsDic = {'nop': 0x01, 'iadd': 0x02, 'isub': 0x05, 'iand': 0x08, 
'ior': 0x0B, 'dup': 0x0E, 'pop':0x10, 'swap': 0x13, 'bipush': 0x19, 
'iload': 0x1C, 'istore': 0x22, 'wide': 0x28, 'ldc_w': 0x32, 'iinc': 0x36, 
'goto': 0x3C, 'iflt': 0x43, 'ifeq': 0x47, 'if_icmpeq':0x4B, 
'invokevirtual':0x55, 'ireturn':0x6B} #Dicionário - guarda os bytes correspondentes a cada instrução (conforme está na específicação passada pelo professor)

bytePos = 0 #Esta variável terá duas funções. A primeira será a de contar
            #o tamanho do programa. A segunda será a de guardar a posição
            #em bytes de cada instrução. 

labelDict = {} #Este dicionário irá guardar a posição, em bytes, dos labels que encontrar

checkLater = {} #Aqui ficarão guardadas as instruções que têm labels como operandos, mas cuja posição do label ainda não é conhecida
for code in assembly:
    mo = assemblyRegex.search(code) #Procura pelo padrão regex na string code
    instruction = mo.group(3)       #Cada parêntesis do regex é um 'grupo'. O grupo 3 é a string com a instrução.
    operando = mo.group(4)          #O operando, se existir, está no grupo 4.
    label = mo.group(2)             #O label, se existir, está no grupo 2. 
    bytePos = bytePos + 1           #As posições das instruções, em byte, começa em 1.
    if(label != None):
        label = label[:len(label)-1]
        labelDict[label] = bytePos   #Se o label existir na string, adicione-o no dicionário de labels
    if(instruction != None):
        memory.append(byteInstructionsDic[instruction]) 
        if(instruction == 'bipush'):
            memory.append(int(operando)) #Como operando é uma string, ele precisa ser transformado num int
            bytePos = bytePos + 1
        elif(instruction == 'ldc_w' or instruction == 'invokevirtual'):
            memory.append( ( int(operando) >> 8) & 255)
            memory.append(int(operando) & 255)
            bytePos = bytePos + 2
        elif(instruction == 'iload' or instruction == 'istore' or instruction == 'iinc'): #iload e istore são duas instruções que recebem variáveis como operandos
            if(operando in varDict): #Se a variável já existir no dicionário, simplesmente adicione a memory o valor dela
                memory.append(varDict[operando])
                
            else:
                varDict[operando] = numVars #Caso contrário, crie uma entrada no dicionário em que a chave é a variável nova e o valor é numVars
                memory.append(numVars)
                numVars = numVars + 1

            bytePos = bytePos + 1

            if(instruction == 'iinc'):
                operando2 = mo.group(5)
                memory.append(int(operando2))
                bytePos = bytePos + 1
        elif(instruction == 'if_icmpeq' or instruction == 'goto' or instruction == 'ifeq' or instruction == 'iflt'): #Estas duas instruções recebem OFFSET como operando. 
                                                                   #Um offset tem, necessariamente, dois bytes. Esse offset pode ser dado como uma label ou 
                                                                   #como um número de dois bytes. Aqui, tratei apenas o caso em que o operando é dado como um label
            if(isNumber(operando)):
                memory.append( ( int(operando) >> 8) & 255)
                memory.append(int(operando) & 255)
            else:
                memory.append(0x00)
                memory.append(0x00)
                checkLater[bytePos] = operando
            bytePos = bytePos + 2
    else:
        memory.append(0x00)



for k in checkLater: 
    byteOfLabel = labelDict[checkLater[k]]
    byteOfInstruction = k
    byteToBeInserted = byteOfLabel - byteOfInstruction 
    memory[k+1] = byteToBeInserted & 255
    memory[k] = (byteToBeInserted >> 8) & 255

tamanhoProg = bytePos + 20 #Tamanho do programa: 20 bytes de inicialização + programa propriamente dito
bytesTamanho = [(tamanhoProg >> 8*i) & 255 for i in range(4)] #Aqui estou dividindo o número tamanhoProg em 4 bytes, que serão inseridos no início do programa montado

sp = 0x1001 + numVars
bytesSP = [(sp >> 8*i) & 255 for i in range(4)]

with open('prog.exe', 'wb') as asm:
    asm.write(bytearray([bytesTamanho[0], bytesTamanho[1], bytesTamanho[2], bytesTamanho[3], 0x00, 0x73, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, bytesSP[0], bytesSP[1], bytesSP[2], bytesSP[3]])) #Bytes de inicialização
    asm.write(memory)
    
    
