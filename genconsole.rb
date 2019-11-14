require 'yaml'
require 'pp'
require 'ostruct'
require './genutil.rb'

scheme = YAML.load_file('console.yml').to_o
@enums = []

def typedecl(parent, name, type)
    if type.is_a?(Array)
        decl = "console_command_#{ parent }_#{ name }"
        @enums << OpenStruct.new({
            :parent => parent,
            :name => name,
            :fields => type,
            :decl => decl
        })
        "#{ decl }_t"
    else
        type
    end
end

def argdefault(arg)
    if arg.default[0] == '$'
        "self->#{ arg.default[1..-1] }"
    else
        case arg.type
        when 'string'
            "\"#{ arg.default }\""
        when 'int'
            arg.default
        end
    end
end

def argparse(parent, arg, i)
    if arg.type.is_a?(Array)
        fields = []
        decl = "console_command_#{ parent }_#{ arg.name }"
        fields << "#{ decl }_t #{ arg.name };"
        arg.type.each_with_index do |field, j|
            compalias = field.alias ? "strncmp(argv[#{i}], \"#{field.alias}\", 256) == 0 || " : ''
            compname = "strncmp(argv[#{i}], \"#{field.name}\", 256) == 0"
            maybe_else = "#{ j > 0 ? 'else ' : ''}"
            assignment = "#{ arg.name} = #{(decl + '_' + field.name).upcase};"

            fields << "#{ maybe_else }if (#{compalias}#{ compname }) { #{ assignment } }"
        end
        fields << "else { console_display(self, \"unknown %s subcommand: %s\", \"#{ parent }\", argv[#{i}]); return; }"
        fields
    else
        case arg.type
        when 'int'
            lhs = "int #{ arg.name }"
            parser = "((argv[#{ i }][0] == '$') ? strtoul(&argv[#{ i }][1], NULL, 16) : atoi(argv[#{ i }]))"
            if arg.default.nil?
                ["#{ lhs } = #{ parser };"]
            else
                ["#{ lhs } = (argc <= #{ i }) ? #{ argdefault(arg) } : #{ parser };"]
            end
        when 'string'
            lhs = "const char *#{ arg.name }"
            if arg.default.nil?
                ["#{ lhs } = argv[#{ i }];"]
            else
                ["#{ lhs } = (argc <= #{ i }) ? #{ argdefault(arg) } : argv[#{ i }];"]
            end
        end
    end
end

coms = scheme.commands.map do |e|
    com = OpenStruct.new(e)
    com.args = com.args || []
    declargs = com.args.map{ |arg| "#{ typedecl(com.name, arg.name, arg.type) } #{ arg.name }" }

    raise "arguments must be less than 8" if declargs.size >= 8

    com.fname = "console_command_#{ com.name }"
    impl_signature = "void #{ com.fname }(console_ref self#{ declargs.map{ |e| ', ' + e }.join })"

    com.enum = "CONSOLE_COMMAND_TYPE_#{ com.name.upcase }"
    com.argparses = com.args.map.with_index{ |arg, i| argparse(com.name, arg, i) }
    com.impl_signature = impl_signature
    com.impl_call = "#{ com.fname }(self#{ com.args.map{ |arg| ', ' + arg.name }.join });"
    com
end

ErbalT.new({
    :enums => @enums,
    :coms => coms,
}).render('console_command_exec.template.c')

ErbalT.new({
    :enums => @enums,
    :coms => coms,
}).render('console_command.template.h')

def interpret_inputs(inputs)
    def interpret_key(rules)
        def interpret_char(ch)
            if ch =~ /('.+')/
                [Regexp.last_match[1]]
            elsif ch =~ /<(.+)>/
                case Regexp.last_match[1]
                when 'ESC'; [27]
                when 'DELETE'; [127]
                when 'NUM'; (0..9).map{|i| "'#{i}'"}
                else; [Regexp.last_match[1]]
                end
            else
                ["'#{ch}'"]
            end
        end

        rules && rules.split(/\s*,\s*/).map do |rule|
            if rule =~ /<CTRL>\s*\+\s*(\S+)/
                interpret_char(Regexp.last_match[1]).map{|c| "0x1f & #{ c }"}
            else
                interpret_char(rule)
            end
        end.flatten
    end

    def interpret_actions(is_default, str)
        str = str
            .gsub('$height', 'self->view_height')
            .gsub('$y', 'self->view_y')
            .gsub('$middle_y', 'self->middle_y')
            .gsub('$input', 'ch')
            .gsub('$pc', 'self->pc_phys')

        str.split(/\s*;\s*/).map do |action|
            m = /([^(\s]+)\s*(?:\((.*)\))?/.match(action)
            "console_command_#{ m[1] }(self#{ (m[2] || "").split(/\s*,\s*/).map{ |p| ', ' + p }.join });"
        end
    end

    inputs.map do |e|
        input = OpenStruct.new(e)
        input.is_default = e.key.nil?
        input.cases = interpret_key(input.key)
        input.actions = interpret_actions(input.is_default, input.action)
        input
    end
end

ErbalT.new({
    :normal_mode_inputs =>
        interpret_inputs(scheme.normal_mode),
    :command_mode_inputs =>
        interpret_inputs(scheme.command_mode),
    :jump_mode_inputs =>
        interpret_inputs(scheme.jump_mode),
}).render('console_input.template.c')
