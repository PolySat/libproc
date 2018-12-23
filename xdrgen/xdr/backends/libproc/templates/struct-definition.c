:: def nullstr(val, pre, post):
::    if val == None or val == "":
::       return 'NULL'
::    #endif
::    return pre + val + post 
:: #enddef
static struct XDR_FieldDefinition ${st.name.replace('::','_',400)}_Fields[] = {
:: for m in st.members:
::    if m.type == 'void':
::       continue
::    #endif
   { (XDR_Decoder)&${types[m.type]['dec']},
      (XDR_Encoder)&${types[m.type]['enc']},
      offsetof(struct ${st.name.replace('::','_',400)}, ${m.name}),
      ${nullstr(m.documentation.key, '"', '"')}, ${nullstr(m.documentation.name, '"', '"')}, ${nullstr(m.documentation.unit, '"', '"')}, ${m.documentation.offset}, ${m.documentation.divisor},
      ${nullstr(types[m.type]['print'], '&', '')}, ${nullstr(types[m.type]['scan'], '&', '')},
      ${nullstr(types[m.type]['field_dealloc'], '&', '')}, ${types[m.type]['id']},
      ${nullstr(m.documentation.description, '"', '"')} },

:: #endfor
   { NULL, NULL, 0, NULL, NULL, NULL, 0, 0, NULL, 0, NULL }
};

static struct XDR_StructDefinition ${st.name.replace('::','_',400)}_Struct = {
   ${st.id.replace('::','_',400)}, sizeof(struct ${st.name.replace('::','_',400)}),
   &XDR_struct_encoder, &XDR_struct_decoder, ${st.name.replace('::','_',400)}_Fields,
   &XDR_malloc_allocator, &${types[st.name]['deallocator']}, &XDR_print_fields_func
};

