import os

def isValidForEdit(v):
    if ('//' in v) or ('/*' in v) or ('*/' in v) or ('&' in v):
        return False
    return True

def hasSubstring(v, target):
    v = v.strip().lower().replace(' ', '').split('(')
    return (v[0] == target)

def extractFuncParams(name, line, fn=None):
    start = line.find(name)
    end = len(line) - line[::-1].find(')') - 1
    params = line[start:end].strip().replace('{}('.format(name), '').split(',')
    if fn:        
        return '{} {} {}'.format(line[:start], fn(params), line[end:])
    return params
