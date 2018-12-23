:: def nullstr(val, pre, post):
::    if val == None or val == "":
::       return 'NULL'
::    #endif
::    return pre + val + post 
:: #enddef
   { ${cmd.id.replace('::','_',400)}, ${cmd.param.replace('::','_',400)},
     ${nullstr(cmd.name, '"', '"')},
     ${nullstr(cmd.summary, '"', '"')},
     NULL, NULL, NULL },
