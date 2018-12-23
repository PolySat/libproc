:: def nullstr(val, pre, post):
::    if val == None or val == "":
::       return 'NULL'
::    #endif
::    return pre + val + post 
:: #enddef
   { ${err.id.replace('::','_',400)}, ${nullstr(err.name, '"', '"')},
     ${nullstr(err.description, '"', '"')} },
