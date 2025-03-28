
  class Api::SensorController < ApplicationController
    def create
      ax = params[:ax]
      ay = params[:ay]
      az = params[:az]
      pressure = params[:pressure]

      pitch_factor = Math.sqrt(ax * ax + ay * ay + az * az)
      roll = Math.atan2(ay, az) * (180 / Math::PI)
      pitch = Math.atan2(-ax, Math.sqrt(ay * ay + az * az)) * (180 / Math::PI)

      response_data = {
        pitch_factor: pitch_factor,
        roll: roll,
        pitch: pitch,
        pressure: pressure,
        instrument: "piano"
      }

      client = OpenAI::Client.new(access_token: ENV["OPENAI_API_KEY"])

      prompt = <<~PROMPT
        Create a one-bar music track based on the following parameters:
        - Pitch factor: #{response_data[:pitch_factor]}
        - Roll angle: #{response_data[:roll]} degrees
        - Pitch angle: #{response_data[:pitch]} degrees
        - Pressure level: #{response_data[:pressure]}
        - Instrument type: #{response_data[:instrument]}
      PROMPT

      response = client.chat(
        parameters: {
          model: "gpt-4",
          messages: [
            { role: "system", content: "音楽を生成します" },
            { role: "user", content: prompt }
          ],
          max_tokens: 300,
          temperature: 0.7
        }
      )

      render json: {
        **response_data,
        generated_music: response.dig("choices", 0, "message", "content")
      }
    rescue => e
      render json: { error: "Internal Server Error", details: e.message }, status: :internal_server_error
    end
  end
