#!/usr/bin/python3
#
# Obtain list of function names from load map
#
import  sys
import  os

def save_rtn (fOut, sRtn):
    if ((not sRtn.startswith ('_'))
        and ( not sRtn.startswith ('stub_'))
        and (not sRtn.endswith ('_shim'))
        and (not sRtn.endswith ('_shims'))
        and ('.' not in sRtn)):
        fOut.write (sRtn + '\n')

def map_funcs (sIn, sOut):
    if ( not os.path.exists (sIn) ):
        print ('Map file ' + sIn + ' does not exist. Using old symbols.')
        if ( not os.path.exists (sOut) ):
            open (sOut, 'w').close ()
        return
    with open (sIn, 'r') as fIn, open (sOut, 'w') as fOut:
        fOut.write ('# ' + sOut + ' - Public routines for inclusion in BBC BASIC SYM table\n'
                    '# Automatically generated by map_funcs.py ' + sIn + ' ' + sOut + '\n'
                    '# Do not edit\n#\n')
        bInMap = False
        bInText = False
        for sLine in fIn:
            sLine = sLine.strip ()
            if ( bInMap ):
                if ( sLine.startswith ('.text') ):
                    bInText = True
                elif (( bInText ) and ( sLine.startswith ('0x') )):
                    lParts = sLine.split ()
                    if ( len (lParts) == 2 ):
                        save_rtn (fOut, lParts[1])
                else:
                    bInText = False
            elif ( sLine == 'Linker script and memory map' ):
                bInMap = True

if ( len (sys.argv) == 3 ):
    map_funcs (sys.argv[1], sys.argv[2])
else:
    print ('Usage: map_funcs.py <map file> <symbols file>')
    sys.exit (1)
