class Api::SensorController < ApplicationController
  def create
    # `sensor_data` 内のデータを配列として許可
    sensor_params = params.require(:sensor_data).map do |sensor|
      sensor.permit(:time, :ax, :ay, :az, :pressure)
    end

    # 各データポイントごとに pitch_data を計算
    pitch_data_list = sensor_params.map { |sensor| calculate_pitch_data(sensor) }

    # OpenAI API 呼び出し
    openai_response = generate_music(pitch_data_list)

    # JSON レスポンスを返す
    render json: { pitch_data: pitch_data_list, generated_music: openai_response }
  rescue => e
    render json: { error: "Internal Server Error", details: e.message }, status: :internal_server_error
  end

  private

  def calculate_pitch_data(sensor)
    ax, ay, az, pressure = sensor.values_at(:ax, :ay, :az, :pressure)

    {
      time: sensor[:time].to_f,
      pitch_factor: Math.sqrt(ax.to_f**2 + ay.to_f**2 + az.to_f**2),
      roll: Math.atan2(ay.to_f, az.to_f) * (180 / Math::PI),
      pitch: Math.atan2(-ax.to_f, Math.sqrt(ay.to_f**2 + az.to_f**2)) * (180 / Math::PI),
      pressure: pressure.to_f,
      instrument: "piano"
    }
  end

  def generate_music(pitch_data_list)
    api_key = ENV["OPENAI_API_KEY"]
    raise "OpenAI API key is missing" unless api_key

    client = OpenAI::Client.new(access_token: api_key)

    # 直近のデータ（最新の10個程度）をプロンプトに反映
    recent_data = pitch_data_list.last(10)

    prompt = <<~PROMPT
      Generate a one-bar MIDI note sequence based on these recent sensor readings:
      #{recent_data.map.with_index { |data, i|# {' '}
          "- Time: #{data[:time]}s, Pitch factor: #{data[:pitch_factor]}, Roll: #{data[:roll]}, Pitch: #{data[:pitch]}, Pressure: #{data[:pressure]}"
      }.join("\n")}

      Music Style and Characteristics:
      - Tempo: 120 BPM
      - Time Signature: 4/4
      - Genre: Classical piano
      - The sequence should contain 4 to 8 notes, with varying pitch, velocity, and duration.
      - Notes should be dynamic and fit within a single bar.

      Format the response as a JSON array of notes, each containing:
      - "pitch" (MIDI pitch value)
      - "velocity" (note intensity)
      - "duration" (in seconds)
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
