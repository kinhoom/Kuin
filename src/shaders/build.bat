@echo off
set fxc_path="C:\Program Files (x86)\Windows Kits\8.1\bin\x64\fxc.exe"
%fxc_path% tri_vs.fx /T vs_4_0 /Fo tri.vs
%fxc_path% tri_ps.fx /T ps_4_0 /Fo tri.ps
%fxc_path% font_ps.fx /T ps_4_0 /Fo font.ps
%fxc_path% rect_vs.fx /T vs_4_0 /Fo rect.vs
%fxc_path% circle_vs.fx /T vs_4_0 /Fo circle.vs
%fxc_path% circle_ps.fx /T ps_4_0 /Fo circle.ps
%fxc_path% tex_vs.fx /T vs_4_0 /Fo tex.vs
%fxc_path% tex_rot_vs.fx /T vs_4_0 /Fo tex_rot.vs
%fxc_path% tex_ps.fx /T ps_4_0 /Fo tex.ps
%fxc_path% obj_vs.fx /T vs_4_0 /Fo obj.vs
%fxc_path% obj_vs.fx /T vs_4_0 /Fo obj_dbg.vs /DDBG
%fxc_path% obj_vs.fx /T vs_4_0 /Fo obj_joint.vs /DJOINT
%fxc_path% obj_vs.fx /T vs_4_0 /Fo obj_joint_dbg.vs /DJOINT /DDBG
%fxc_path% obj_ps.fx /T ps_4_0 /Fo obj.ps
%fxc_path% obj_ps.fx /T ps_4_0 /Fo obj_dbg.ps /DDBG
pause
