# This grammar is written using Pyparsing and closely follows the grammar in
# RFC 4506.
#
# Known differences with the standard:
#  - We don't allow inline struct, enum, and union definitions.
#  - The standard is inconsistent on whether identifiers are allowed as enum
#    values. We only support constants as enum values.

## Grammar

import os
import pyparsing as P
from xdr.ir import *

class Parser:
   def namespaceParse(self, t):
      if t[1].upper() == 'IPC':
         self.namespace = 'IPC::'
      else:
         if self.parent == None:
#Enums defined in the parent IPC scope
            self.scopeMap['ipc::cmds::status'] = 'IPC::CMDS::STATUS'
            self.scopeMap['ipc::cmds::data_req'] = 'IPC::CMDS::DATA_REQ'
            self.scopeMap['ipc::datareq'] = 'IPC::DataReq'
            self.namespace = 'IPC::' + t[1].upper() + '::'
         else:
            self.namespace = self.parent.namespace + t[1].upper() + '::'
      return t

   def scopeIdent(self, t):
      return self.namespace + t[0]

   def scopeIdentDecl(self, t):
      self.scopeMap[t[0].lower()] = self.namespace + t[0]
      return self.namespace + t[0]

   def scopeIdentUpper(self, t):
      return self.namespace + t[0].upper()

   def enumStart(self, t):
      self.enumNamespace = self.namespace + t[0].upper() + '::'
      self.scopeMap[t[0].lower()] = self.namespace + t[0].upper()
      return self.namespace + t[0].upper()

   def enumIdent(self, t):
      if isinstance(t[0], str):
         name = self.enumNamespace + str(t[0]).upper()
         self.scopeMap[name[len(self.namespace):].lower()] = name
         return name
      return t[0]

   def newStruct(self, t):
      self.scopeMap[t[1].split('::')[-1].lower()] = t[1]

   def resolveIdentStr(self, ident):
      if ident in self.scopeMap:
         return self.scopeMap[ident]
      if self.parent == None:
         raise NameError(ident + ' undefined')
      else:
         return self.parent.resolveIdentStr(ident)

   def resolveIdent(self, t):
      return self.resolveIdentStr(t[0].lower())

   def __init__(self, filename, parentParser):
      self.parent = parentParser
      self.filename = filename
      self.namespace = 'FOO_'
      self.cmdcnt = 0
      self.enumNamespace = ''
      self.scopeMap = {}
      kw = P.Keyword
      s = P.Suppress
      lit = P.Literal
      g = P.Group

# -:  self + And._ErrorStop() + other
      identifier = P.Word(P.alphanums, P.alphanums + '_').setName("identifier")

      scopedidentifier = P.Word(P.alphanums, P.alphanums + '_')
      scopedidentifier.setParseAction(self.scopeIdent)

      newscopedidentifier = P.Word(P.alphanums, P.alphanums + '_')
      newscopedidentifier.setParseAction(self.scopeIdentDecl)

      scopedUpperIdentifier = P.Word(P.alphanums, P.alphanums + '_')
      scopedUpperIdentifier.setParseAction(self.scopeIdentUpper)

      enumIdentifier = P.Word(P.alphanums, P.alphanums + '_')
      enumIdentifier.setParseAction(self.enumStart)

      enumValueIdentifier = P.Word(P.alphanums, P.alphanums + '_')
      enumValueIdentifier.setParseAction(self.enumIdent)

      resolvedIdentifier = P.Word(P.alphanums, P.alphanums + '_')
      resolvedIdentifier.setParseAction(self.resolveIdent)

      type_name = P.Combine(identifier -  P.OneOrMore('::' + identifier))
      type_name.setParseAction(self.resolveIdent)

      decimal_constant = P.Word('-123456789', '0123456789') | '0'
      hexadecimal_constant = P.Combine('0x' - P.Word('0123456789abcdefABCDEF'))
      octal_constant = P.Word('0', '01234567')

      constant = (hexadecimal_constant | decimal_constant) \
          .setName("constant") \
          .setParseAction(lambda x: int(x[0], 0))

      value = constant | identifier

      struct_body = P.Forward()

      type_specifier = \
          (P.Optional(kw("unsigned")) + kw("int")) | \
          (P.Optional(kw("unsigned")) + kw("hyper")) | \
          kw("float") | kw("double") | kw("quadruple") | kw("bool") | \
          kw("char") | \
          s(kw("struct")) + struct_body | \
          identifier

      resolved_type_specifier = \
          (P.Optional(kw("unsigned")) + kw("int")) | \
          (P.Optional(kw("unsigned")) + kw("hyper")) | \
          kw("float") | kw("double") | kw("quadruple") | kw("bool") | \
          kw("char") | \
          s(kw("struct")) + struct_body | \
          resolvedIdentifier

      case_type_specifier = \
          (P.Optional(kw("unsigned")) + kw("int")) | \
          (P.Optional(kw("unsigned")) + kw("hyper")) | \
          kw("float") | kw("double") | kw("quadruple") | kw("bool") | \
          kw("char") | \
          scopedidentifier

      declaration = \
          kw("void") | \
          kw("opaque") + identifier + lit("[") + value + lit("]") | \
          kw("opaque") + identifier + lit("<") + P.Optional(value) + lit(">") | \
          kw("string") + identifier + lit("<") + P.Optional(value) + lit(">") | \
          g(resolved_type_specifier) + identifier + lit("[") + value + lit("]") | \
          g(resolved_type_specifier) + identifier + lit("<") + P.Optional(value) + lit(">") | \
          g(resolved_type_specifier) + identifier | \
          g(resolved_type_specifier) + lit('*') + identifier

      fielddocumentation = \
          s("{") + (P.Optional(g(kw("key") + identifier + s(";"))) & \
             P.Optional(g(kw("name") + P.QuotedString('"') + s(";"))) & \
             P.Optional(g(kw("unit") + P.QuotedString('"') + s(";"))) & \
             P.Optional(g(kw("offset") + decimal_constant + s(";"))) & \
             P.Optional(g(kw("divisor") + decimal_constant + s(";"))) & \
             P.Optional(g(kw("description") + P.QuotedString('"') + s(";"))) \
          ) + s("}")


      const_expr_value = constant | scopedidentifier
      constant_expr = (const_expr_value + (kw('+') | kw('-')) + const_expr_value) | const_expr_value
      enum_body = s("{") + g(P.delimitedList(g(enumValueIdentifier + s('=') + constant_expr))) + s("}")
      
      struct_body << s("{") + P.OneOrMore(g(declaration + g(P.Optional(fielddocumentation)) + s(";"))) + s("}")

      constant_def = kw("const") - scopedUpperIdentifier - s("=") - constant - s(";")
      namespace_def = kw("namespace") - identifier - s(";")
      namespace_def.setParseAction(self.namespaceParse)

      struct_def = kw("struct") - newscopedidentifier - g(struct_body) + P.Optional(s('=') - type_name) - s(";")
      struct_def.setParseAction(self.newStruct)

      command_options = \
             P.Optional(g(kw("summary") + P.QuotedString('"') + s(";"))) & \
             P.Optional(g(kw("param") + type_name + s(";")))  & \
             P.Optional(g(kw("types") + s("=") + g(type_name + P.ZeroOrMore(s(',') + type_name)) + s(";"))) & \
             P.Optional(g(kw("response") + g(type_name + P.ZeroOrMore(s(',') + type_name)) + s(";")))

      command_body = s("{") + command_options + s("}")
      
      command_def = kw("command") - P.QuotedString('"') - g(command_body) - P.Optional(s('=') + type_name) - s(";")

      union_def = kw("union") - newscopedidentifier - s('{') + s('}') - s(";")

      error_def = kw("error") - type_name - s('=') + P.QuotedString('"') + s(";")

      type_def = \
          kw("enum") - enumIdentifier - enum_body - s(";") | \
          union_def | \
          struct_def | \
          command_def | \
          error_def

      import_def = kw("import") - P.QuotedString('"') - s(";")

      definition = type_def | constant_def | import_def

      self.specification = g(namespace_def) & P.ZeroOrMore(g(definition))

      self.specification.ignore(P.cStyleComment)

   def xdr_parse_ast(self, src):
      """
      Given an input string, return the AST.
      """
      return self.specification.parseString(src, parseAll=True).asList()

   def xdr_parse_fielddoc(self, x):
      key = ''
      name = ''
      desc = ''
      offset = divisor = 0
      unit = ''
      for field in x:
         if field[0] == 'key':
            key = field[1]
         if field[0] == 'name':
            name = field[1]
         if field[0] == 'description':
            desc = field[1]
         if field[0] == 'unit':
            unit = field[1]
         if field[0] == 'offset':
            offset = field[1]
         if field[0] == 'divisor':
            divisor = field[1]
      return XDRFieldDocumentation(key, name, desc, offset, divisor, unit)

   def xdr_parse_declaration(self, x):
      if x[0] == 'void':
         return XDRDeclaration(None, 'basic', 'void', None, None, True, XDRFieldDocumentation('', '', '', 0, 0, ''))
      elif x[0] == 'opaque' or x[0] == 'string':
         type = x[0]
         name = x[1]
         kind = x[2] == '[' and 'array' or 'list'
         length = x[3]
         docs = None
         if length == '>':
            length = None
            if len(x) > 4:
               docs = self.xdr_parse_fielddoc(x[4])
         else:
            if len(x) > 5:
               docs = self.xdr_parse_fielddoc(x[5])

         if length == None:
            length_type = ''
            length_const = True
         else:
            try:
               intlen = int(length)
               length_type = 'fixed'
               length_const = True
            except:
               length_type = 'variable'
               length_const = False
         if type == 'string':
            length_type = 'variable'

         return XDRDeclaration(name, kind, type, length, length_type, length_const, docs)
      else:
         name = x[1]
         docidx = 2
         kind = 'basic'
         type = ' '.join(x[0])
         length = None
         if name == '*':
            name = x[2]
            kind = 'optional'
            docidx = 3
         elif len(x) > 2 and x[2] == '[':
            kind = 'array'
         elif len(x) > 2 and x[2] == '<':
            kind = 'list'
         if kind == 'array' or kind == 'list':
            length = x[3]
            docidx = 5
            if length == '>':
               length = None
               docidx = 4
                
         doc = None
         if len(x) > docidx:
            doc = self.xdr_parse_fielddoc(x[docidx])
         if length == None:
            length_type = ''
            length_const = True
         else:
            try:
               intlen = int(length)
               length_type = 'fixed'
               length_const = True
            except:
               length_type = 'variable'
               length_const = False
         return XDRDeclaration(name, kind, type, length, length_type, length_const, doc)

   def xdr_parse_definition(self, x):
      if x[0] == 'namespace':
         return [XDRNamespace(x[1])]
      elif x[0] == 'import':
         fn = os.path.join(os.path.dirname(self.filename), x[1])
         p = Parser(fn, self)
         with open(fn, 'r') as importfile:
            data=importfile.read()
         return p.xdr_parse(data)
      elif x[0] == 'struct':
         sid = None
         if len(x) == 4:
            sid = x[3]
         return [XDRStruct(x[1], [self.xdr_parse_declaration(y) for y in x[2]], sid)]
      elif x[0] == 'enum':
         return [XDREnum(x[1], [XDREnumMember(y[0], ' '.join(str(e) for e in y[1:])) for y in x[2]])]
      elif x[0] == 'const':
         return [XDRConst(x[1], ' '.join(str(e) for e in x[2:]))]
      elif x[0] == 'typedef':
         return [XDRTypedef(self.xdr_parse_declaration(x[1]))]
      elif x[0] == 'union':
         return [XDRUnion(x[1], None, [])]
#        return XDRUnion(x[1], self.xdr_parse_declaration(x[2]),
#                        [XDRUnionMember(y[0], self.xdr_parse_declaration(y[1])) for y in x[3]])
      elif x[0] == 'error':
         return [XDRError(x[1], x[1].split('::')[-1], x[2])]

      elif x[0] == 'command':
         summary = None
         param = '0'
         response = None
         types = None
         num = '0'
         for field in x[2]:
            if field[0] == 'summary':
               summary = field[1]
            if field[0] == 'param':
               param = field[1]
            if field[0] == 'response':
               response = field[1]
            if field[0] == 'types':
               types = field[1]
         if len(x) >= 4:
            num = x[3]
         self.cmdcnt += 1
         return [XDRCommand(x[1], num, summary, param, response, types, \
               self.namespace + 'AUTOCMD_' + str(self.cmdcnt))]

   def xdr_parse(self, src):
      """
      Given an input string, return the IR.
      """
      ast = self.xdr_parse_ast(src)
#print(ast)
#print(self.scopeMap)
      ir = []
      for x in ast:
         ir = ir + self.xdr_parse_definition(x)
#      print(ir)
      return ir

