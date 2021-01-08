import sys
if len(sys.argv)>2:
    with open(str(sys.argv[1]), 'r') as fichier1:
        with open(str(sys.argv[2]), 'r') as fichier2:
            i=0
            while 1:
                char1 = fichier1.read(1)
                char2 = fichier2.read(1)
                if not char1: break
                if not char2: break
                if(char1!=char2):
                    print("line " + str(i) + ": " + str(ord(char1)) + " "+ str(ord(char2)))
                i+=1
else:
    print("missing arguments")