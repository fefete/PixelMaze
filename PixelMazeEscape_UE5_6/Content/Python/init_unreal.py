try:
    import setup_pixelmaze_assets
    setup_pixelmaze_assets.ensure_assets()
except Exception as exc:
    import unreal
    unreal.log_error(f"Pixel Maze automatic asset setup failed: {exc}")
    unreal.log_error("Run Content/Python/setup_pixelmaze_assets.py manually from Tools > Execute Python Script.")
