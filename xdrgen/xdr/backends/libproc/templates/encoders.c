int ${st.name.replace('::','_',400)}_decode(char *src,
      struct ${st.name.replace('::','_',400)} *dst, size_t *used,
      size_t max)
{
   return XDR_struct_decoder(src, dst, used, max, ${st.name.replace('::','_',400)}_Fields);
}

int ${st.name.replace('::','_',400)}_encode(
      struct ${st.name.replace('::','_',400)} *src, char *dst, size_t *used,
      size_t max)
{
   return XDR_struct_encoder(src, dst, used, max,
         ${st.id.replace('::','_',400)} , ${st.name.replace('::','_',400)}_Fields);
}

