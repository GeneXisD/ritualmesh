# lib/invoker.rb
class Invoker
  def initialize(mesh)
    @mesh = mesh
    @commands = {
      "oracle" => -> { oracle },
      "summon" => -> { summon },
      "whoami" => -> { whoami },
    }
  end

  def invoke(cmd)
    if @commands.key?(cmd)
      @commands[cmd].call
    else
      @mesh.log "Unknown invocation: #{cmd}"
    end
  end

  def oracle
    @mesh.log "The Oracle sees a thousand timelines converge…"
  end

  def summon
    @mesh.log "Summoning ritual modules…"
    Dir["./modules/*.rb"].each do |mod|
      require_relative "../#{mod}"
      klass = File.basename(mod, ".rb").capitalize
      if Object.const_defined?(klass)
        Object.const_get(klass).new(@mesh).summon
      else
        @mesh.log "Module #{klass} not found."
      end
    end
  end

  def whoami
    @mesh.log "You are the architect. Your mesh ID is #{@mesh.object_id}"
  end
end

