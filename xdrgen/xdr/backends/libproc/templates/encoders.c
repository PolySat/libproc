int ${st.name.replace('::','_',400)}_decode(char *src,
      struct ${st.name.replace('::','_',400)} *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_decoder(src, dst, used, max, ${st.name.replace('::','_',400)}_Fields);
}

int ${st.name.replace('::','_',400)}_encode(
      struct ${st.name.replace('::','_',400)} *src, char *dst, size_t *used,
      size_t max, void *len)
{
   return XDR_struct_encoder(src, dst, used, max,
         ${st.id.replace('::','_',400)} , ${st.name.replace('::','_',400)}_Fields);
}

int ${st.name.replace('::','_',400)}_decode_array(char *src,
      struct ${st.name.replace('::','_',400)} **dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_decoder(src, (char*)dst, used, max, *(int32_t*)len,
            sizeof(struct ${st.name.replace('::','_',400)}),
            (XDR_Decoder)&${st.name.replace('::','_',400)}_decode, NULL);

   return 0;
}

int ${st.name.replace('::','_',400)}_encode_array(
      struct ${st.name.replace('::','_',400)} **src, char *dst, size_t *used,
      size_t max, void *len)
{
   *used = 0;
   if (len)
      return XDR_array_encoder((char*)src, dst, used, max, *(int32_t*)len,
            sizeof(struct ${st.name.replace('::','_',400)}),
            (XDR_Encoder)&${st.name.replace('::','_',400)}_encode, NULL);

   return 0;
}

