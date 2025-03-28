class Api::ButtonController < ApplicationController
  # メモリ内の擬似データ（実際のデータベースに接続しない場合）
  BUTTONS = {
    1 => { name: "Button1", status: true },
    2 => { name: "Button2", status: true },
    3 => { name: "Button3", status: true }
  }

  def update
    button_id = params[:id].to_i
    if BUTTONS[button_id]
      # `status` をトグル（ON/OFFを切り替え）
      BUTTONS[button_id][:status] = !BUTTONS[button_id][:status]
      render json: BUTTONS[button_id], status: :ok
    else
      render json: { error: "Button not found" }, status: :not_found
    end
  end
end
