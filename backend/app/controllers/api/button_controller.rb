class Api::ButtonController < ApplicationController
  # メモリ内の擬似データ（実際のデータベースに接続しない場合）
  BUTTONS = {
    1 => { name: "Start-Stop", status: false },
    2 => { name: "Button2", status: true },
    3 => { name: "Button3", status: true }
  }

  # ボタン1が押された最初の時間を保持
  @@button_1_start_time = nil

  def update
    button_id = params[:id].to_i

    if BUTTONS[button_id]
      # `status` をトグル（ON/OFFを切り替え）
      BUTTONS[button_id][:status] = !BUTTONS[button_id][:status]

      # ボタン1の場合、経過時間を計算してセッションに保存
      if button_id == 1
        current_time = Time.now

        if @@button_1_start_time.nil?


          # 新しいSessionを作成
          session = Session.create(
            date: Date.today,
            duration: 0.0,
            heltzh: 0.0,
            yaw_up: 0.0,
            pitch_left: 0.0,
            pressure_avg: 0.0,
            overpressure_count: 0,
            total_vibration: 0.0
          )
          if session.save
            @@button_1_start_time = current_time
            render json: { status: "OK", session_id: session.id }, status: :ok
          else
            render json: { error: "Failed to create session" }, status: :unprocessable_entity
          end
        else
          # 経過時間を計算
          elapsed_time = (current_time - @@button_1_start_time).to_i
          @@button_1_start_time = nil
          session = Session.last

          if session
            if elapsed_time < 20
              # 経過時間が20秒未満の場合はセッションを削除
              session.destroy
              render json: { status: "OK", message: "Session deleted because elapsed time was less than 20 seconds" }, status: :ok
            else
              # セッションに経過時間を追加
              session.duration += elapsed_time
              session.heltzh = session.total_vibration / session.duration
              session.save
              render json: { status: "OK", session_id: session.id, elapsed_time: elapsed_time }, status: :ok
            end
          else
            render json: { error: "Session not found" }, status: :not_found
          end
        end
      else
        render json: BUTTONS[button_id], status: :ok
      end
    else
      render json: { error: "Button not found" }, status: :not_found
    end
  end
end
