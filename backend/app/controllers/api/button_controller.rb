
  class Api::ButtonController < ApplicationController
    def create
      action = params[:button] == "button1" ? "reset" : "stop"
      render json: { status: "ok", action: action }
    end
  end
