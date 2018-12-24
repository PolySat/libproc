extern int ${st.name.replace('::','_',400)}_decode(char *src,
      struct ${st.name.replace('::','_',400)} *dst, size_t *used,
      size_t max, void *len);
extern int ${st.name.replace('::','_',400)}_encode(
      struct ${st.name.replace('::','_',400)} *src, char *dst, size_t *used,
      size_t max, void *len);
extern int ${st.name.replace('::','_',400)}_decode_array(char *src,
      struct ${st.name.replace('::','_',400)} **dst, size_t *used,
      size_t max, void *len);
extern int ${st.name.replace('::','_',400)}_encode_array(
      struct ${st.name.replace('::','_',400)} **src, char *dst, size_t *used,
      size_t max, void *len);

