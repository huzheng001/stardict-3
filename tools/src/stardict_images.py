#!/usr/bin/env python
# -*- coding: utf-8 -*-
from gimpfu import *
import os

def prepare_image(image, visibleLayers, size, numColors = None):
  """prepare custom image
  image - image object to change
  size - size of the image in pixels
  visibleLayers - a list of layers that must be visible
  """
  for layer in image.layers:
    if layer.name in visibleLayers:
      layer.visible = True
    else:
      image.remove_layer(layer)
  gimp.pdb.gimp_image_merge_visible_layers(image, CLIP_TO_IMAGE)
  drawable=gimp.pdb.gimp_image_get_active_layer(image)
  gimp.pdb.gimp_layer_scale_full(drawable, size, size, False, INTERPOLATION_CUBIC)
  """
  image 670x670, all layers have the same dimensions
  after applying gimp_image_scale_full functions with size=32,
  image.width = 32, image.height = 32
  layer.width = 27, layer.height = 31

  gimp.pdb.gimp_image_scale_full(image, size, size, INTERPOLATION_CUBIC)
  """
  #print 'width = {0}, height = {1}'.format(drawable.width, drawable.height)
  #print 'width = {0}, height = {1}'.format(image.width, image.height)
  if numColors != None:
    gimp.pdb.gimp_image_convert_indexed(image, NO_DITHER, MAKE_PALETTE, numColors, False, False, "")

def save_image(image, dstFilePath):
  dirPath = os.path.dirname(dstFilePath)
  if not os.path.exists(dirPath):
    os.makedirs(dirPath)
  drawable=gimp.pdb.gimp_image_get_active_layer(image)
  gimp.pdb.gimp_file_save(image, drawable, dstFilePath, dstFilePath)
  gimp.delete(drawable)
  gimp.delete(image)

def create_icon(origImage, visibleLayers, props):
  """visibleLayers - a list of layers that must be visible
  props - tuple of image properties in format ((size, bpp), ...)
  where: 
    size - size of the icon in pixels,
    bpp - bits per pixel, None to leave by default
  return value - new image
  """
  iconImage = None
  i = 0
  for prop in props:
    image = gimp.pdb.gimp_image_duplicate(origImage)
    prepare_image(image, visibleLayers, prop[0], prop[1])
    image.layers[0].name = 's{0}'.format(i)
    if iconImage == None:
      iconImage = image
    else:
      newLayer = gimp.pdb.gimp_layer_new_from_drawable(image.layers[0], iconImage)
      gimp.pdb.gimp_image_add_layer(iconImage, newLayer, -1)
      gimp.delete(image)
    i += 1
      
  return iconImage
  
def stardict_images(srcFilePath, rootDir):
  if not rootDir:
    # srcFilePath = rootDir + "/pixmaps/stardict.xcf"
    if not srcFilePath.endswith("/pixmaps/stardict.xcf"):
      print('Unable to automatically detect StarDict root directory. Specify non-blank root directory parameter.')
      return
    dstDirPath = os.path.dirname(srcFilePath)
    dstDirPath = os.path.dirname(dstDirPath)
  else:
    dstDirPath = rootDir
  """
  print 'srcFilePath = {0}'.format(srcFilePath)
  print 'rootDir = {0}'.format(rootDir)
  print 'dstDirPath = {0}'.format(dstDirPath)
  """

  dstStarDict_s128_FilePath=os.path.join(dstDirPath, "pixmaps/stardict_128.png")
  dstStarDict_s32_FilePath=os.path.join(dstDirPath, "pixmaps/stardict_32.png")
  dstStarDict_s16_FilePath=os.path.join(dstDirPath, "pixmaps/stardict_16.png")
  dstStarDict_FilePath=os.path.join(dstDirPath, "pixmaps/stardict.png")
  dstStarDictEditor_s128_FilePath=os.path.join(dstDirPath, "pixmaps/stardict-editor_128.png")
  dstStarDictEditor_s32_FilePath=os.path.join(dstDirPath, "pixmaps/stardict-editor_32.png")
  dstStarDictEditor_s16_FilePath=os.path.join(dstDirPath, "pixmaps/stardict-editor_16.png")
  dstStarDictIconFilePath=os.path.join(dstDirPath, "pixmaps/stardict.ico")
  dstStarDictEditorIconFilePath=os.path.join(dstDirPath, "pixmaps/stardict-editor.ico")
  dstStarDictUninstIconFilePath=os.path.join(dstDirPath, "pixmaps/stardict-uninst.ico")
  dstDockletNormalFilePath=os.path.join(dstDirPath, "src/pixmaps/docklet_normal.png")
  dstDockletScanFilePath=os.path.join(dstDirPath, "src/pixmaps/docklet_scan.png")
  dstDockletStopFilePath=os.path.join(dstDirPath, "src/pixmaps/docklet_stop.png")
  dstDockletGPENormalFilePath=os.path.join(dstDirPath, "src/pixmaps/docklet_gpe_normal.png")
  dstDockletGPEScanFilePath=os.path.join(dstDirPath, "src/pixmaps/docklet_gpe_scan.png")
  dstDockletGPEStopFilePath=os.path.join(dstDirPath, "src/pixmaps/docklet_gpe_stop.png")
  dstWordPickFilePath=os.path.join(dstDirPath, "src/win32/acrobat/win32/wordPick.bmp")
  
  origImage=gimp.pdb.gimp_file_load(srcFilePath, srcFilePath)
  
  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2"), 128)
  save_image(image, dstStarDict_s128_FilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2"), 32)
  save_image(image, dstStarDict_s32_FilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2"), 16)
  save_image(image, dstStarDict_s16_FilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2"), 64)
  save_image(image, dstStarDict_FilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2", "edit"), 128)
  save_image(image, dstStarDictEditor_s128_FilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2", "edit"), 32)
  save_image(image, dstStarDictEditor_s32_FilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2", "edit"), 16)
  save_image(image, dstStarDictEditor_s16_FilePath)

  image = create_icon(origImage, ("book1", "book2"),
    ((16, None), (32, None), (48, None), (16, 256), (32, 256), (48, 256), (256, None))
    )
  save_image(image, dstStarDictIconFilePath)
    
  image = create_icon(origImage, ("book1", "book2", "edit"),
    ((16, None), (32, None), (48, None), (16, 256), (32, 256), (48, 256), (256, None))
    )
  save_image(image, dstStarDictEditorIconFilePath)
    
  image = create_icon(origImage, ("book1", "book2", "cross"),
    ((16, None), (32, None), (48, None), (16, 256), (32, 256), (48, 256), (256, None))
    )
  save_image(image, dstStarDictUninstIconFilePath)
    
  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2"), 32)
  save_image(image, dstDockletNormalFilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2", "search"), 32)
  save_image(image, dstDockletScanFilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2", "stop"), 32)
  save_image(image, dstDockletStopFilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2"), 16)
  save_image(image, dstDockletGPENormalFilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2", "search"), 16)
  save_image(image, dstDockletGPEScanFilePath)

  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2", "stop"), 16)
  save_image(image, dstDockletGPEStopFilePath)

  # See AVToolButtonNew function in PDF API Reference
  # Recommended icon size is 18x18, but it looks too small...
  image = gimp.pdb.gimp_image_duplicate(origImage)
  prepare_image(image, ("book1", "book2"), 22)
  gimp.set_background(192, 192, 192)
  gimp.pdb.gimp_layer_flatten(image.layers[0])
  save_image(image, dstWordPickFilePath)

register(
        "stardict_images",
        "Create images for StarDict",
        "Create images for StarDict",
        "StarDict team",
        "GPL",
        "Mar 2011",
        "<Toolbox>/Tools/stardict images",
        "",
        [
          (PF_FILE, "src_image", "Multilayer image used as source for all other images in StarDict, "
            + "normally that is pixmaps/stardict.xcf is StarDict source tree.", None),
          (PF_DIRNAME, "stardict_dir", "Root directory of StarDict source tree. New images will be saved here.", None)
        ],
        [],
        stardict_images)

main()
