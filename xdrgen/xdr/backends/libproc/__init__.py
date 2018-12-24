import os
import tenjin
from xdr.parser import *

type_map = { 'int': { 'type': 'int32_t', 'dec': 'XDR_decode_int32', 'enc':'XDR_encode_int32', 'print': 'XDR_print_field_int32', 'scan': 'XDR_scan_int32', 'dealloc':False, 'id': '0', 'field_dealloc': None }, \
             'unsigned int': { 'type': 'uint32_t', 'dec': 'XDR_decode_uint32', 'enc': 'XDR_encode_uint32', 'print': 'XDR_print_field_uint32', 'scan': 'XDR_scan_uint32', 'dealloc':False, 'id': '0', 'field_dealloc': None }, \
             'float': { 'type': 'float', 'dec': 'XDR_decode_float', 'enc': 'XDR_encode_float', 'print': 'XDR_print_field_float', 'scan': 'XDR_scan_float', 'dealloc':False, 'id': '0', 'field_dealloc': None }, \
             'void': { 'type': 'uint32_t', 'dec': 'XDR_decode_uint32', 'enc': 'XDR_encode_uint32', 'print': 'XDR_print_field_uint32', 'scan': 'XDR_scan_uint32', 'dealloc':False, 'id': '0', 'field_dealloc': None }, \
             'hyper': { 'type': 'int64_t', 'dec': 'XDR_decode_int64', 'enc': 'XDR_encode_int64', 'print': 'XDR_print_field_int64', 'scan': 'XDR_scan_int64', 'dealloc':False, 'id': '0', 'field_dealloc': None}, \
             'unsigned hyper': { 'type': 'uint64_t', 'dec': 'XDR_decode_uint64', 'enc': 'XDR_encode_uint64', 'print': 'XDR_print_field_uint64', 'scan': 'XDR_scan_uint64', 'dealloc':False, 'id': '0', 'field_dealloc': None }, \
             'string': { 'type': 'char', 'dec': 'XDR_decode_string', 'enc': 'XDR_encode_string', 'print': 'XDR_print_field_string', 'scan': 'XDR_scan_string', 'dealloc':True, 'deallocator': 'XDR_dealloc_string', 'id': '0', 'field_dealloc': None }, \
             'opaque': { 'type': 'char', 'dec': 'XDR_decode_byte', 'enc': 'XDR_encode_byte', 'print': 'XDR_print_field_byte', 'scan': 'XDR_scan_byte', 'dealloc':True, 'deallocator': 'XDR_dealloc_byte', 'id': '0', 'field_dealloc': None }, \
           }

def extract_namespace(ir, default, prefix):
   for x in ir:
      if isinstance(x, XDRNamespace):
         if x.name.upper() == prefix:
            return x.name.upper()
         return prefix + '_' + x.name
   return prefix + default

def extract_union_mapping(ir, namespace):
   map = {}
   for x in ir:
      if isinstance(x, XDRUnion) and x.name.split('::')[-1] == 'Data':
         for m in x.members:
            if not m.declaration.type == 'void':
               map[m.declaration.type] = m.case
      if isinstance(x, XDRUnion) and x.name.split('::')[-1] == 'CommandParameters':
         for m in x.members:
            if not m.declaration.type == 'void':
               map[m.declaration.type] = m.case
   return map

def setup_types(ir):
   for x in ir:
      if isinstance(x, XDRUnion):
         type_map[x.name] = { 'type': 'struct XDR_Union', 'dec': 'XDR_decode_union', 'enc': 'XDR_encode_union', 'print': '', 'scan': '', 'dealloc': True, 'deallocator': 'XDR_dealloc_union', 'id': '0', 'field_dealloc': None }
      elif isinstance(x, XDREnum):
         type_map[x.name] = { 'type': 'uint32_t', 'dec': 'XDR_decode_uint32', 'enc': 'XDR_encode_uint32', 'print': 'XDR_print_field_uint32', 'scan': 'XDR_scan_uint32', 'dealloc': False, 'id': '0', 'field_dealloc': None }
      elif isinstance(x, XDRStruct):
         dealloc = False;
         deallocator = 'XDR_free_deallocator'
         for m in x.members:
            dealloc = dealloc or type_map[m.type]['dealloc']
         if dealloc:
            deallocator = x.name.replace('::','_',400) + "_dealloc"
         type_map[x.name] = { 'type': 'struct ' + x.name.replace('::','_',400), 'dec':  x.name.replace('::','_',400) + "_decode", \
            'enc':  x.name.replace('::','_',400) + "_encode", 'print': '', 'scan': '', 'dealloc': dealloc, 'deallocator': 'XDR_struct_free_deallocator', 'id': x.id.replace('::','_',400), 'field_dealloc': 'XDR_struct_field_deallocator' }

def generateSource(ir, output, namespace, mapping):
   out = open(output + ".c", 'w')
   render_template(out, "header.c", dict(namespace=namespace,header=output.split('/')[-1] + ".h"))

   for x in ir:
      if isinstance(x, XDRStruct):
         render_template(out, "struct-definition.c", dict(st=x,types=type_map,enums=mapping))

   for x in ir:
      if isinstance(x, XDRStruct):
         render_template(out, "encoders.c", dict(st=x,types=type_map,enums=mapping))
   render_template(out, "command-header.c", dict(ns=namespace))
   for x in ir:
      if isinstance(x, XDRCommand):
         render_template(out, "command-definition.c", dict(cmd=x,types=type_map,enums=mapping))
   render_template(out, "command-footer.c", dict(ns=namespace))

   render_template(out, "error-header.c", dict(ns=namespace))
   for x in ir:
      if isinstance(x, XDRError):
         render_template(out, "error-definition.c", dict(err=x,types=type_map,enums=mapping))
   render_template(out, "error-footer.c", dict(ns=namespace))

   render_template(out, "register.c", dict(namespace=namespace))
   for x in ir:
      if isinstance(x, XDRStruct):
         render_template(out, "register-struct.c", dict(st=x))
   repl = 1;
   if namespace == "IPC":
       repl = 0
   render_template(out, "register-footer.c", dict(namespace=namespace, repl=repl))

   out.close()

def generateHeader(ir, output, namespace, mapping):
   out = open(output + ".h", 'w')
   out.write("#ifndef " + output.split('/')[-1].upper() + "_H\n")
   out.write("#define " + output.split('/')[-1].upper() + "_H\n\n")
   out.write("#include <polysat/xdr.h>\n")
   out.write("#include <polysat/cmd.h>\n")
   out.write("#include <stdint.h>\n\n")

   for x in ir:
      if isinstance(x, XDRConst):
         render_template(out, "const.h", dict(const=x,types=type_map))
   for x in ir:
      if isinstance(x, XDREnum):
         render_template(out, "enum.h", dict(enum=x,types=type_map))
   for x in ir:
      if isinstance(x, XDRStruct):
         render_template(out, "struct.h", dict(st=x,types=type_map))
   for x in ir:
      if isinstance(x, XDRStruct):
         render_template(out, "prototypes.h", dict(st=x,types=type_map))

   render_template(out, "footer.h", dict(namespace=namespace))
   out.close()

def generate(ir, output):
#print(ir)
    namespace = extract_namespace(ir, output, 'IPC')
    mapping = extract_union_mapping(ir, namespace)
    setup_types(ir)
#print(type_map)

    generateHeader(ir, output, namespace, mapping)
    generateSource(ir, output, namespace, mapping)

def render_template(out, name, context):
#print(context)
    path = [os.path.join(os.path.dirname(os.path.realpath(__file__)), 'templates')]
    pp = [ tenjin.PrefixedLinePreprocessor() ] # support "::" syntax
    template_globals = { "to_str": str, "escape": str } # disable HTML escaping
    engine = tenjin.Engine(path=path, pp=pp)
    out.write(engine.render(name, context, template_globals))
