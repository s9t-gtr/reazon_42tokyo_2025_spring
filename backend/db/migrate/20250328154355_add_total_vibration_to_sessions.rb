class AddTotalVibrationToSessions < ActiveRecord::Migration[8.0]
  def change
    add_column :sessions, :total_vibration, :float
  end
end
