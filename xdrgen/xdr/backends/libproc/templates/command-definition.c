:: def nullstr(val, pre, post):
::    if val == None or val == "":
::       return 'NULL'
::    #endif
::    return pre + val + post 
:: #enddef
:: if cmd.types == None:
   { ${cmd.id.replace('::','_',400)}, ${cmd.param.replace('::','_',400)},
:: else:
   { 0, IPC_TYPES_DATAREQ,
:: #endif
     ${nullstr(cmd.name, '"', '"')},
     ${nullstr(cmd.summary, '"', '"')},
:: if cmd.types == None:
     NULL,
:: else:
     ${cmd.autoname.replace('::','_',400)}_types,
:: #endif
     NULL, NULL, NULL },
