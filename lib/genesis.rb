# lib/genesis.rb
class Genesis
  def initialize(mesh)
    @mesh = mesh
  end

  def activate
    @mesh.log "GENESIS activated. Binding ritual keys…"
    @fingerprint = OpenSSL::Digest::SHA256.hexdigest(@mesh.object_id.to_s)
    @mesh.log "GENESIS ID: #{@fingerprint}"
  end
end

