enum ${enum.name.replace('::','_',400)} {
:: for m in enum.members:
   ${m.name.replace('::','_',400)} = ${str(m.value).replace('::','_',400)},
:: #endfor
};

