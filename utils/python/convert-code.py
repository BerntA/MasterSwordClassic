"""
Convert some ancient C/C++ syntax to comp under a newer vers.
"""

import os

def getForLoop(v):
    return 'for (int {} = 0; {} < {}; {}++)'.format(v[0], v[0], v[1], v[0])

def checkAndConvFile(f):
    should_update, content = False, []
    with open(f, 'r') as c:
        for line in c:
            if ('foreach' in line) and (not '//' in line) and (not ';' in line):                
                should_update = True
                start = line.find('foreach')
                end = len(line) - line[::-1].find(')') - 1
                loop = getForLoop(line[start:end].strip().replace(' ', '').replace('foreach(', '').split(','))                
                line =  '{} {} {}'.format(line[:start], loop, line[(end+1):])
                #print(line)
            content.append(line)
       
    #should_update = False
    #print(''.join(content))
    
    if should_update:
        print("Writing:", f)
        with open(f, 'w') as c:
            c.write(''.join(content))

def convertBogusCode():
    types = set(['cpp', 'h', 'c'])
    for root, subdirs, files in os.walk(r'E:\Cloud\GIT\MasterSwordClassic'):
        if '.git' in root:
            continue

        for f in files:
            f = '{}\\{}'.format(root, f)
            if not (f.lower().split('.')[-1] in types):
                continue
            try:
                checkAndConvFile(f)
                print('Processed:', f)
            except Exception as e:
                print('ERROR:', f, '-->', e)
            #break

if __name__ == "__main__":
    convertBogusCode()
