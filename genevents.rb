require './genutil.rb'

scheme = YAML.load_file('events.yml').to_o


scheme.events.each do |d|
    d.actions.each do |a|
        a.enums = []
        a.args = a.args || []
        a.args.each do |arg|
            if arg.type.is_a? Array
                decl = "#{ d.domain }_event_#{ a.name }_#{ arg.name }"
                a.enums << OpenStruct.new({
                    :fields => arg.type,
                    :decl => decl,
                })
                arg.type = "#{ decl }_t"
            end
        end
        a.id = "EVENT_ID_#{ d.domain.upcase }_#{ a.name.upcase }"
        a.elem_decl = "_#{ d.domain }_event_listener_on_#{ a.name }"
        a.callback_decl = "#{ d.domain }_event_callback_on_#{ a.name }_t"
    end

    d.emitter_decl = "#{ d.domain }_event_emitter"
    d.listener_decl = "#{ d.domain }_event_listener"
    d.includes = d.includes || []

    ErbalT.new(d).render("event_listener.template.h", "#{d.domain}_event_listener.h")

    ErbalT.new(d).render("event_emitter.template.h", "#{d.domain}_event_emitter.h")
end

ErbalT.new(scheme).render("events.template.h")
ErbalT.new(scheme).render("event_id.template.h")
