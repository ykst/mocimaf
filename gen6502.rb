require './genutil.rb'

cases = []
code_cache = {}
profiles = []

File.readlines('6502.tsv').each_with_index do |line, idx|
    next if idx == 0

    elem = line.strip.split(/\t/)
    profile = OpenStruct.new
    op = elem[0]
    num = elem[0].to_i(16)
    name = elem[1]
    memory = elem[2]
    official = elem[3] == "T"
    pchfix = elem[4] == "T"
    mode = elem[5]
    code = elem[6]

    enum = "OP_#{ op }_#{ name }#{ official ? '' : '_UNOFFICIAL'}"

    profile.op = op
    profile.num = num
    profile.enum = enum
    profile.name = name
    profile.mode = mode
    profile.official = official
    profile.pchfix = pchfix
    profile.code = code
    profile.memory = memory

    case mode
    when 'IMPLIED','ACCUMULATOR'; length = 1
    when 'ABSOLUTE', 'ABSOLUTE_X', 'ABSOLUTE_Y', 'INDIRECT'; length = 3
    else; length = 2
    end

    profile.length = length
    profiles << profile

    impl = nil
    fetch = "fetch_#{ mode.downcase }"

    gen = []

    if memory == 'W' || mode == 'IMPLIED'
        # none
    elsif mode == 'IMPLIED' || mode == 'ACCUMULATOR' || memory == 'RMW' || memory == 'W'
        gen << "uint16_t tmp;"
    else
        gen << "uint16_t tmp = value;"
    end

    if code
        code.split(/,\s*/).each do |elem|
            m = /([^(.]+)(?:\((.*)\))?/.match(elem)
            action = m[1]
            content_raw = (m[2] || 'T')
            content = content_raw
                .gsub('PULL16','cpu_pull16(self)')
                .gsub('PULL','cpu_pull(self)')
                .gsub('PC','self->reg.pc')
                .gsub('T','tmp')
                .gsub('O','value')
                .gsub('M','addr')
                .gsub('R','cpu_read(self, addr)')
                .gsub('C','self->reg.carry')
                .gsub('V','self->reg.overflow')
                .gsub('N','self->reg.negative')
                .gsub('Z','self->reg.zero')
                .gsub('S','self->reg.s')
                .gsub('X','self->reg.x')
                .gsub('Y','self->reg.y')
                .gsub('P','self->reg.p')
                .gsub('A','self->reg.a')

            gen << "// #{action}(#{content_raw})"

            case action
            when 'T'; gen << "tmp = #{ content };"
            when 'M'; gen << "addr = #{ content };"
            when 'C'; gen << "self->reg.carry = (bool)(#{ content });"
            when 'N'; gen << "self->reg.negative = ((#{ content }) >> 7) & 1;"
            when 'Z'; gen << "self->reg.zero = !((#{ content }) & 0xff);"
            when 'I'; gen << "self->reg.int_disable = (bool)(#{ content });"
            when 'V'; gen << "self->reg.overflow = (bool)(#{ content });"
            when 'D'; gen << "self->reg.decimal = (bool)(#{ content });"
            when 'W'; gen << "cpu_write(self, addr, #{ content });"
            when 'R'; gen << "(void)cpu_read(self, #{ content });"
            when 'B'; gen << "cpu_decode_branch(self, addr, #{ content });"
            when 'A'; gen << "self->reg.a = #{ content };"
            when 'P'; gen << "self->reg.p = #{ content };"
            when 'X'; gen << "self->reg.x = #{ content };"
            when 'S'; gen << "self->reg.s = #{ content };"
            when 'Y'; gen << "self->reg.y = #{ content };"
            when 'G'; gen << "bus_clock_cpu(self->shared_bus, #{ content });"
            when 'PC'; gen << "self->reg.pc = #{ content };"
            when 'VEC'; gen << "self->reg.pc = cpu_read_vector(self, #{ content });"
            when 'PUSH16'; gen << "cpu_push16(self, #{ content });"
            when 'PUSH'; gen << "cpu_push(self, #{ content });"
            when 'IRQ_DELAY'; gen << "self->irq_poll_delay = #{ content };"
            when 'UNUSED'; gen << "(void)(#{content});"
            when 'NOTIMPL'; gen << "WARN(\"#{name}[#{op}] not implemented\\n\");"
            else
                if /@\s*([0-9A-F]{2})/ =~ action && code_cache.has_key?(Regexp.last_match[1]) && impl.nil?
                    impl = code_cache[Regexp.last_match[1]]
                    code_cache[op] = impl
                else
                    throw "undefined action: #{action}"
                end
            end
        end

        if impl.nil?
            fname = "cpu_#{ official ? 'decode' : 'decode_unofficial' }_#{op}_#{ name.downcase }"
            if memory == "W" || memory == "RMW"
                impl = "#{fname}(self, addr);"
                signature = "static inline void #{fname}(cpu_t *self, uint16_t addr)"
            elsif mode == 'IMPLIED' || mode == 'ACCUMULATOR'
                impl = "#{fname}(self);"
                signature = "static inline void #{fname}(cpu_t *self)"
            else
                impl = "#{fname}(self, value);"
                signature = "static inline void #{fname}(cpu_t *self, uint8_t value)"
            end
            code_cache[op] = impl

            profile.gencode = gen

            profile.fname = signature
            profile.func = <<-EOS
// [#{name}] OP:#{op} Mode:#{mode} Length:#{length}
// Semantics:#{code}
#{signature}
{
#{indent gen.join("\n")}
}
EOS
        end
    end

    codes = []
    memory_hint = pchfix || (mode.include?('INDIRECT') && mode != 'INDIRECT') || mode.include?('ABSOLUTE')

    case memory
    when '-'
        case mode
        when 'IMPLIED', 'ACCUMULATOR'
            codes << (impl || "// NOP")
            codes << "// fetch next instruction and throw it away"
            codes << "(void)cpu_read(self, self->reg.pc);"
        else
            if code
                codes << "uint8_t value = #{ fetch }(self);" << impl
            else
                codes << "(void)#{ fetch }(self);" << "// NOP"
            end
        end
    when 'R'
        codes << "uint16_t addr = #{ fetch }(self#{ memory_hint ? ', true' : ''});"
        if code
            codes << "uint8_t value = cpu_read(self, addr);" << impl
        else
            codes << "(void)cpu_read(self, addr);" << "// NOP"
        end
    when 'W', 'RMW'
        codes << "uint16_t addr = #{ fetch }(self#{ memory_hint ? ', false' : '' });"
        codes << (impl || "// NOP")
    end

    cases << indent([
        "case #{ enum }: {",
        indent(codes.join("\n")),
        "    break;",
        "}"
    ].join("\n"))

end

ErbalT.new({
    :profiles => profiles.select{|p| p.official && p.gencode}
}).render('cpu_decode.template.h', 'cpu_decode_official.h')

ErbalT.new({
    :profiles => profiles.select{|p| !p.official && p.gencode}
}).render('cpu_decode.template.h', 'cpu_decode_unofficial.h')

ErbalT.new({
    :cases => cases
}).render('cpu_decode_internal.template.h')

ErbalT.new({
    :profiles => profiles,
}).render('cpu_opcode.template.h')

ErbalT.new({
    :profiles => profiles
}).render('cpu_opcode.template.c')

