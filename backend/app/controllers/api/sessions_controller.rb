module Api
  class SessionsController < ApplicationController
    # GET /api/sessions
    def index
      # date パラメータがある場合、それを基にフィルタリング
      if params[:start_date].present? && params[:end_date].present?
        # start_date と end_date を使ってデータを取得
        @sessions = Session.where(date: params[:start_date]..params[:end_date])
      else
        # date パラメータがない場合は全てのデータを取得
        @sessions = Session.all
      end

      render json: @sessions
    end

    # GET /api/sessions/:id
    def show
      @session = Session.find(params[:id])
      render json: @session
    rescue ActiveRecord::RecordNotFound
      render json: { error: "Session not found" }, status: :not_found
    end

    # POST /api/sessions
    def create
      @session = Session.new(session_params)
      if @session.save
        render json: @session, status: :created
      else
        render json: @session.errors, status: :unprocessable_entity
      end
    end

    private

    def session_params
      params.require(:session).permit(:date, :duration, :heltzh, :yaw_up, :pitch_left, :pressure_avg, :overpressure_count)
    end
  end
end
