:: def nullstr(val, pre, post):
::    if val == None or val == "":
::       return 'NULL'
::    #endif
::    return pre + val + post 
:: #enddef
:: if cmd.types != None:
static uint32_t ${cmd.autoname.replace('::','_',400)}_types[] = {
   ${str(cmd.types).replace('[','',400).replace(']','',400).replace("'",'',400).replace('::','_',400)}, 0
};

:: #endif
