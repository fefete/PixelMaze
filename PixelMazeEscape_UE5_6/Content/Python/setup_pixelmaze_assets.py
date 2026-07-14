"""Create the editable Unreal assets used by Pixel Maze Escape.

The project ships source PNG and WAV files rather than opaque .uasset binaries.
This script imports textures and audio, applies pixel-art settings, creates the
unlit gameplay materials and marks both BGM SoundWaves as looping. It is safe
to run repeatedly.
"""

from pathlib import Path
import unreal

DEST_TEXTURES = "/Game/PixelMaze/Textures"
DEST_MATERIALS = "/Game/PixelMaze/Materials"
DEST_CHARACTER_MATERIALS = "/Game/PixelMaze/Materials/Characters"
DEST_AUDIO = "/Game/PixelMaze/Audio"

MATERIAL_ASSET_NAMES = ("Floor", "Wall", "Player", "Player2", "Enemy", "Exit", "Fog")
CHARACTER_ASSET_NAMES = tuple(
    f"Character_{index:02d}" for index in range(1, 11)
)
EXTRA_TEXTURES = {
    "Icon": "icon.png",
    "TitleScreen": "title_screen.png",
    "Tileset": "tileset_4x1.png",
}
CHARACTER_SOURCE_FILES = {
    f"Character_{index:02d}": f"Characters/character_{index:02d}.png"
    for index in range(1, 11)
}

SOURCE_FILES = {
    "Floor": "floor.png",
    "Wall": "wall.png",
    "Player": "player.png",
    "Player2": "player2.png",
    "Enemy": "enemy.png",
    "Exit": "exit.png",
    "Fog": "fog.png",
    **EXTRA_TEXTURES,
    **CHARACTER_SOURCE_FILES,
}

AUDIO_FILES = {
    "BGM_Menu_80sSoft_Loop": "BGM_Menu_80sSoft_Loop.wav",
    "BGM_Maze_BreathingMaze_Loop": "BGM_Maze_BreathingMaze_Loop.wav",
    "SFX_Player_Step": "SFX_Player_Step.wav",
    "SFX_Victory": "SFX_Victory.wav",
    "SFX_Defeat": "SFX_Defeat.wav",
    "SFX_Enemy_Step": "SFX_Enemy_Step.wav",
    "SFX_Enemy_Vocal": "SFX_Enemy_Vocal.wav",
    "SFX_Enemy_HitPlayer": "SFX_Enemy_HitPlayer.wav",
}
LOOPING_AUDIO = {
    "BGM_Menu_80sSoft_Loop",
    "BGM_Maze_BreathingMaze_Loop",
}
SPATIAL_ENEMY_AUDIO = {
    "SFX_Enemy_Step",
    "SFX_Enemy_Vocal",
}
ENEMY_ATTENUATION_PATH = f"{DEST_AUDIO}/ATT_Enemy.ATT_Enemy"


def _enum_value(enum_type, *candidate_names):
    for name in candidate_names:
        if hasattr(enum_type, name):
            return getattr(enum_type, name)
    return None


def _project_source_dir(folder_name: str) -> Path:
    return Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())) / folder_name


def _source_art_dir() -> Path:
    return _project_source_dir("SourceArt")


def _source_audio_dir() -> Path:
    return _project_source_dir("SourceAudio")


def _texture_path(name: str) -> str:
    return f"{DEST_TEXTURES}/T_{name}.T_{name}"


def _material_path(name: str) -> str:
    destination = (
        DEST_CHARACTER_MATERIALS
        if name in CHARACTER_ASSET_NAMES
        else DEST_MATERIALS
    )
    return f"{destination}/M_{name}.M_{name}"


def _sound_path(name: str) -> str:
    return f"{DEST_AUDIO}/SW_{name}.SW_{name}"


def _create_enemy_attenuation():
    attenuation = unreal.load_asset(ENEMY_ATTENUATION_PATH)
    if not attenuation:
        factory = unreal.SoundAttenuationFactory()
        attenuation = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name="ATT_Enemy",
            package_path=DEST_AUDIO,
            asset_class=unreal.SoundAttenuation,
            factory=factory,
        )

    if not attenuation:
        unreal.log_error("Pixel Maze: failed to create ATT_Enemy")
        return None

    try:
        settings = attenuation.get_editor_property("attenuation")
        settings.set_editor_property("attenuate", True)
        settings.set_editor_property("spatialize", True)
        settings.set_editor_property("attenuation_shape_extents", unreal.Vector(150.0, 0.0, 0.0))
        settings.set_editor_property("falloff_distance", 1100.0)

        linear = _enum_value(unreal.AttenuationDistanceModel, "LINEAR")
        sphere = _enum_value(unreal.AttenuationShape, "SPHERE")
        if linear is not None:
            settings.set_editor_property("distance_algorithm", linear)
        if sphere is not None:
            settings.set_editor_property("attenuation_shape", sphere)

        attenuation.set_editor_property("attenuation", settings)
    except Exception as exc:
        unreal.log_warning(f"Pixel Maze: enemy attenuation settings warning: {exc}")

    unreal.EditorAssetLibrary.save_loaded_asset(attenuation)
    return attenuation


def _import_texture(name: str):
    texture = unreal.load_asset(_texture_path(name))
    if not texture:
        filename = _source_art_dir() / SOURCE_FILES.get(name, f"{name.lower()}.png")
        if not filename.exists():
            unreal.log_error(f"Pixel Maze: missing source art file: {filename}")
            return None

        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(filename))
        task.set_editor_property("destination_path", DEST_TEXTURES)
        task.set_editor_property("destination_name", f"T_{name}")
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("save", True)

        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        texture = unreal.load_asset(_texture_path(name))

    if not texture:
        unreal.log_error(f"Pixel Maze: failed to import texture {name}")
        return None

    nearest = _enum_value(unreal.TextureFilter, "NEAREST", "TF_NEAREST")
    no_mips = _enum_value(unreal.TextureMipGenSettings, "NO_MIPMAPS", "TMGS_NO_MIPMAPS")
    texture_group = _enum_value(unreal.TextureGroup, "PIXELS2D", "TEXTUREGROUP_PIXELS2D")

    try:
        if nearest is not None:
            texture.set_editor_property("filter", nearest)
        if no_mips is not None:
            texture.set_editor_property("mip_gen_settings", no_mips)
        if texture_group is not None:
            texture.set_editor_property("lod_group", texture_group)
        texture.set_editor_property("srgb", True)
        texture.set_editor_property("never_stream", True)
    except Exception as exc:
        unreal.log_warning(f"Pixel Maze: some texture properties could not be set for {name}: {exc}")

    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    return texture


def _create_material(name: str, texture):
    existing = unreal.load_asset(_material_path(name))
    if existing:
        return existing

    factory = unreal.MaterialFactoryNew()
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name=f"M_{name}",
        package_path=(
            DEST_CHARACTER_MATERIALS
            if name in CHARACTER_ASSET_NAMES
            else DEST_MATERIALS
        ),
        asset_class=unreal.Material,
        factory=factory,
    )
    if not material:
        unreal.log_error(f"Pixel Maze: failed to create material M_{name}")
        return None

    try:
        material.set_editor_property("two_sided", False)
        material.set_editor_property("blend_mode", _enum_value(unreal.BlendMode, "OPAQUE", "BLEND_OPAQUE"))
        unlit = _enum_value(unreal.MaterialShadingModel, "UNLIT", "MSM_UNLIT")
        if unlit is not None:
            material.set_editor_property("shading_model", unlit)
    except Exception as exc:
        unreal.log_warning(f"Pixel Maze: material base settings warning for {name}: {exc}")

    sample = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionTextureSample,
        -360,
        -20,
    )
    sample.set_editor_property("texture", texture)

    emissive_property = _enum_value(unreal.MaterialProperty, "MP_EMISSIVE_COLOR", "EMISSIVE_COLOR")
    if emissive_property is not None:
        unreal.MaterialEditingLibrary.connect_material_property(sample, "RGB", emissive_property)

    try:
        usage = _enum_value(
            unreal.MaterialUsage,
            "MATUSAGE_INSTANCED_STATIC_MESHES",
            "INSTANCED_STATIC_MESHES",
        )
        if usage is not None:
            unreal.MaterialEditingLibrary.set_material_usage(material, usage)
    except Exception as exc:
        unreal.log_warning(f"Pixel Maze: could not pre-enable instanced mesh usage for {name}: {exc}")

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def _import_sound(name: str, source_filename: str, enemy_attenuation=None):
    sound = unreal.load_asset(_sound_path(name))
    if not sound:
        filename = _source_audio_dir() / source_filename
        if not filename.exists():
            unreal.log_error(f"Pixel Maze: missing source audio file: {filename}")
            return None

        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(filename))
        task.set_editor_property("destination_path", DEST_AUDIO)
        task.set_editor_property("destination_name", f"SW_{name}")
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", False)
        task.set_editor_property("save", True)

        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        sound = unreal.load_asset(_sound_path(name))

    if not sound:
        unreal.log_error(f"Pixel Maze: failed to import sound {name}")
        return None

    try:
        sound.set_editor_property("looping", name in LOOPING_AUDIO)
        if name in SPATIAL_ENEMY_AUDIO and enemy_attenuation:
            sound.set_editor_property("attenuation_settings", enemy_attenuation)
    except Exception as exc:
        unreal.log_warning(f"Pixel Maze: sound properties could not be set for {name}: {exc}")

    unreal.EditorAssetLibrary.save_loaded_asset(sound)
    return sound


def ensure_assets(force_rebuild: bool = False):
    unreal.EditorAssetLibrary.make_directory(DEST_TEXTURES)
    unreal.EditorAssetLibrary.make_directory(DEST_MATERIALS)
    unreal.EditorAssetLibrary.make_directory(DEST_CHARACTER_MATERIALS)
    unreal.EditorAssetLibrary.make_directory(DEST_AUDIO)

    if force_rebuild:
        for name in MATERIAL_ASSET_NAMES + CHARACTER_ASSET_NAMES:
            unreal.EditorAssetLibrary.delete_asset(_material_path(name).split(".")[0])
            unreal.EditorAssetLibrary.delete_asset(_texture_path(name).split(".")[0])
        for name in EXTRA_TEXTURES:
            unreal.EditorAssetLibrary.delete_asset(_texture_path(name).split(".")[0])
        for name in AUDIO_FILES:
            unreal.EditorAssetLibrary.delete_asset(_sound_path(name).split(".")[0])
        unreal.EditorAssetLibrary.delete_asset(ENEMY_ATTENUATION_PATH.split(".")[0])

    enemy_attenuation = _create_enemy_attenuation()

    created = []
    for name in MATERIAL_ASSET_NAMES:
        texture = _import_texture(name)
        if texture:
            material = _create_material(name, texture)
            if material:
                created.append(name)

    for name in CHARACTER_ASSET_NAMES:
        texture = _import_texture(name)
        if texture:
            material = _create_material(name, texture)
            if material:
                created.append(name)

    for name in EXTRA_TEXTURES:
        if _import_texture(name):
            created.append(name)

    for name, filename in AUDIO_FILES.items():
        if _import_sound(name, filename, enemy_attenuation):
            created.append(f"SW_{name}")

    unreal.EditorAssetLibrary.save_directory(
        "/Game/PixelMaze",
        only_if_is_dirty=False,
        recursive=True,
    )
    unreal.log(
        f"Pixel Maze: asset setup complete "
        f"({', '.join(created) or 'assets already existed'})."
    )
    return created


if __name__ == "__main__":
    ensure_assets()
