"""
Convert some ancient C/C++ syntax to comp under a newer vers.
"""

import os
import re
from utils import isValidForEdit, extractFuncParams, hasSubstring

def getStrncpy(v):
    v[0] = v[0].strip().replace(' ', '').replace('\r', '').replace('\t', '')
    return 'strncpy({}, {}, sizeof({})'.format(v[0], ', '.join(v[1:]), v[0])

def getSnprintf(v):    
    if len(v) == 2:
        return getStrncpy(v)
    v[0] = v[0].strip().replace(' ', '').replace('\r', '').replace('\t', '')
    return '_snprintf({}, sizeof({}), {}, {}'.format(v[0], v[0], v[1], ', '.join(v[2:]))        

def checkAndConvFile(f):
    should_update, content = False, []
    with open(f, 'r') as c:
        for line in c:
            if isValidForEdit(line):
                if hasSubstring(line, 'strcpy'):
                    should_update = True
                    line = extractFuncParams('strcpy', line, lambda x: getStrncpy(x))
                elif hasSubstring(line, 'sprintf'):
                    should_update = True
                    line = extractFuncParams('sprintf', line, lambda x: getSnprintf(x))
            content.append(line)
       
    #should_update = False
    if should_update:
        print("Writing:", f)
        with open(f, 'w') as c:
            c.write(''.join(content))

def convertBogusCode():
    types = set(['cpp', 'h', 'c'])
    excluded = set(['engine', 'public', 'utils', 'dedicated', 'common', 'thirdparty', '.git', 'vpc_scripts', 'debug_', 'release_', 'devtools'])    
    for root, subdirs, files in os.walk(r'E:\Cloud\GIT\MasterSwordClassic'):
        if len([True for ex in excluded if ('/{}'.format(ex) in root.lower().replace('\\', '/'))]):
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
