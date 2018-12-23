struct ${st.name.replace('::','_',400)} {
:: for m in st.members:
::    if m.type == 'void':
   int voidfield;
::       continue
::    #endif
:: if (m.kind == 'array' or m.kind == 'list') and m.length_type == 'fixed':
   ${types[m.type]['type']} ${m.name}[${m.length}];
:: elif m.kind == 'array' or m.kind == 'list':
   ${types[m.type]['type']} *${m.name};
:: else:
   ${types[m.type]['type']} ${m.name};
:: #endif
:: #endfor
};

