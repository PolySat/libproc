import os
from xdr.parser import *
from xdr.backends.xp.template_utils import render_template
#from template_utils import render_template

def collect_constants(ir):
    constants = {}
    for x in ir:
        if isinstance(x, XDRConst):
            assert x.name not in constants
            constants[x.name] = x.value
        elif isinstance(x, XDREnum):
            for m in x.members:
                assert m.name not in constants
                constants[m.name] = m.value
    return constants

def generate(ir, output):
    out = open(output + '.xp', 'w')

#    constants = collect_constants(ir)

    render_template(out, "header.xp", {})

    for x in ir:
        out.write("\n")
        if isinstance(x, XDRConst):
            render_template(out, "const.xp", dict(const=x))
        elif isinstance(x, XDREnum):
            render_template(out, "enum.xp", dict(enum=x))
        elif isinstance(x, XDRError):
            render_template(out, "error.xp", dict(err=x))
        elif isinstance(x, XDRCommand):
            idval = ' = ' + x.id
            if x.id == "0":
                idval = ""
            render_template(out, "command.xp", dict(cmd=x,idval=idval))
        elif isinstance(x, XDRStruct):
            render_template(out, "struct-header.xp", dict(struct=x))
            for m in x.members:
                arr = ""
                if m.length != None:
                    if m.type == 'string':
                        arr = '<' + str(m.length) + '>';
                    else:
                        arr = '[' + str(m.length) + ']';
                render_template(out, "struct-member.xp", dict(m=m,arr=arr))

            render_template(out, "struct-footer.xp", dict(struct=x))
        elif isinstance(x, XDRUnion):
            render_template(out, "union.xp", dict(union=x))
