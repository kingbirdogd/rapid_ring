export PROJECT_HOME:=$(shell pwd|sed "s/\/$$//g;")
include $(PROJECT_HOME)/common.mk
