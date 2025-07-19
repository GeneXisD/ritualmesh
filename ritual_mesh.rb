#!/usr/bin/env ruby
require 'json'
require 'securerandom'
require 'openssl'
require_relative './lib/genesis'
require_relative './lib/invoker'

class RitualMesh
  def initialize
    @id = SecureRandom.uuid
    @genesis = Genesis.new(self)
    @invoker = Invoker.new(self)
    log "Ritual Mesh initialized. ID: #{@id}"
  end

  def run
    @genesis.activate
    loop do
      print "\n⛩️  Invoke > "
      cmd = gets.strip
      break if cmd == 'exit'
      @invoker.invoke(cmd)
    end
  end

  def log(msg)
    puts "[#{Time.now}] :: #{msg}"
  end
end

RitualMesh.new.run

