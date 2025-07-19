# modules/fire.rb
class Fire
  def initialize(mesh)
    @mesh = mesh
  end

  def summon
    @mesh.log "🔥 Element of Fire bound to Ritual Mesh."
  end
end

