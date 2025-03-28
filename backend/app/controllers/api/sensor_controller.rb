class Api::SensorController < ApplicationController
    def create
      # params[:sensor] 内のパラメータを許可する
      sensor_params = params.require(:sensor).permit(:ax, :ay, :az, :pressure)

      # pitch_data の計算
      pitch_data = calculate_pitch_data(sensor_params)

      # OpenAI API 呼び出し
      openai_response = generate_music(pitch_data)

      # JSON レスポンスを返す
      render json: pitch_data.merge(generated_music: openai_response)
    rescue => e
      render json: { error: "Internal Server Error", details: e.message }, status: :internal_server_error
    end

    private

    def calculate_pitch_data(params)
      ax, ay, az, pressure = params.values_at(:ax, :ay, :az, :pressure)

      {
        pitch_factor: Math.sqrt(ax.to_f**2 + ay.to_f**2 + az.to_f**2),
        roll: Math.atan2(ay.to_f, az.to_f) * (180 / Math::PI),
        pitch: Math.atan2(-ax.to_f, Math.sqrt(ay.to_f**2 + az.to_f**2)) * (180 / Math::PI),
        pressure: pressure.to_f,
        instrument: "piano"
      }
    end

    def generate_music(pitch_data)
        api_key = ENV["OPENAI_API_KEY"]
        raise "OpenAI API key is missing" unless api_key

        client = OpenAI::Client.new(access_token: api_key)

        # 詳細なプロンプト
        prompt = <<~PROMPT
          Create a one-bar MIDI note sequence in JSON format based on these parameters:
          - Pitch factor: #{pitch_data[:pitch_factor]}
          - Roll angle: #{pitch_data[:roll]} degrees
          - Pitch angle: #{pitch_data[:pitch]} degrees
          - Pressure level: #{pitch_data[:pressure]}
          - Instrument type: #{pitch_data[:instrument]}

          Music Style and Characteristics:
          - Tempo: Moderate (120 beats per minute).
          - Time signature: 4/4.
          - Genre: Classical piano music, with a focus on a flowing, lyrical melody.
          - The sequence should contain at least four notes, but no more than eight.
          - The notes should start at a mid-range pitch (around 60 to 70) and should gradually move higher or lower.
          - The velocity of the notes should vary slightly to create a natural dynamic.
          - The duration of each note should be consistent or vary slightly, but all notes should be part of the same bar (4 beats).

          Format the response as a JSON array of notes, where each note has "pitch" (MIDI pitch value), "velocity" (how loud the note is), and "duration" (how long the note lasts in seconds).
        PROMPT

        response = client.chat(
          parameters: {
            model: "gpt-4",
            messages: [
              { role: "system", content: "Generate music" },
              { role: "user", content: prompt }
            ],
            max_tokens: 300,
            temperature: 0.7
          }
        )

        response.dig("choices", 0, "message", "content") || "No music generated"
      rescue => e
        Rails.logger.error("OpenAI API error: #{e.message}")
        "Error generating music"
      end
end
