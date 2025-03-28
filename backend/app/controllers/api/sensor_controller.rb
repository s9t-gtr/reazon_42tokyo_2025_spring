class Api::SensorController < ApplicationController
  # センサーから送信されるデータを処理
  def create
    sensor_data = params.require(:sensor_data).map do |sensor|
      sensor.permit(:vibration, :pressure_avg, :pitch_left, :yaw_up)
    end

    # セッションIDをパラメータから取得
    session_id = params[:session_id]
    @session = Session.find_by(id: session_id)


    if @session
      total_vibration = 0
      total_pressure_avg = 0
      total_pitch_left = 0
      total_yaw_up = 0

      # センサーデータをループして合計を計算
      sensor_data.each do |data|
        total_vibration += data[:vibration] if data[:vibration]
        total_pressure_avg += data[:pressure_avg] if data[:pressure_avg]
        total_pitch_left += data[:pitch_left] if data[:pitch_left]
        total_yaw_up += data[:yaw_up] if data[:yaw_up]
      end


      # セッションに新しいデータを反映
      # @session.heltzh += total_heltz
      @session.total_vibration += total_vibration
      @session.pressure_avg += total_pressure_avg
      @session.pitch_left += total_pitch_left
      @session.yaw_up += total_yaw_up



      # セッションを保存
      @session.save

      render json: { status: "OK", session: @session }
    else
      render json: { error: "Session not found" }, status: :not_found
    end
  rescue => e
    render json: { error: "Internal Server Error", details: e.message }, status: :internal_server_error
  end
end
