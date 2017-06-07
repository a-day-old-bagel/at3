
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"
#include "loadedTexture.h"
#include "openglValidation.h"

namespace at3 {

  void at3::LoadedTexture::loadSingleTexture(const std::string &textureFile) {
    if (textureFile.empty()) {
      return;
    }
    int x, y, n;
    stbi_set_flip_vertically_on_load(true);
    uint8_t* data = stbi_load(textureFile.c_str(), &x, &y, &n, 0);
    if (data) {
      GLint oglFormat = 0;
      switch (n) {
        case 3: {
          mStatus = SINGLE_RGB;
          oglFormat = GL_RGB;
        } break;
        case 4: {
          mStatus = SINGLE_RGBA;
          oglFormat = GL_RGBA;
        } break;
        default: {
          fprintf(stderr, "Unsupported number of channels in texture file '%s'.\n", textureFile.c_str());
          stbi_image_free(data);
          return;
        }
      }
      // Create the texture object in the GL
      glGenTextures(1, &mTexture);
      FORCE_ASSERT_GL_ERROR();
      glBindTexture(GL_TEXTURE_2D, mTexture);
      FORCE_ASSERT_GL_ERROR();
      // Copy the image to the GL
      glTexImage2D(
          GL_TEXTURE_2D,  // target
          0,  // level
          oglFormat,  // internal format
          x,  // width
          y,  // height
          0,  // border
          (GLenum)oglFormat,  // format
          GL_UNSIGNED_BYTE,  // type
          data  // data
      );
      FORCE_ASSERT_GL_ERROR();

      glGenerateMipmap(GL_TEXTURE_2D);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);         FORCE_ASSERT_GL_ERROR();
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                       FORCE_ASSERT_GL_ERROR();
    } else {
      fprintf(stderr, "Failed to load texture file '%s'.\n", textureFile.c_str());
    }
    stbi_image_free(data);
  }

  void LoadedTexture::loadCubeTexture(const std::string &textureFileBaseName) {

    // split file an extension names
    size_t lastDotIndex = textureFileBaseName.find_last_of(".");
    std::string rawName = textureFileBaseName.substr(0, textureFileBaseName.find_last_of("."));
    std::string extName = textureFileBaseName.substr(lastDotIndex, textureFileBaseName.size());

    // generate true file names for separate images (one for each side of cube)
    std::string front   = std::string("assets/cubeMaps/" + rawName + "/negz" + extName);
    std::string back    = std::string("assets/cubeMaps/" + rawName + "/posz" + extName);
    std::string bottom  = std::string("assets/cubeMaps/" + rawName + "/negy" + extName);
    std::string top     = std::string("assets/cubeMaps/" + rawName + "/posy" + extName);
    std::string left    = std::string("assets/cubeMaps/" + rawName + "/negx" + extName);
    std::string right   = std::string("assets/cubeMaps/" + rawName + "/posx" + extName);

    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, mTexture);
    FORCE_ASSERT_GL_ERROR();

    // FIXME: Why does top go with negative-y and bottom with positive-y? WHY?
    int r;
#   define CONTINUE_IF_SUCCESS(s, f) r = loadCubeSide(s, f); if (r) return;
    CONTINUE_IF_SUCCESS(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, front);
    CONTINUE_IF_SUCCESS(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, back);
    CONTINUE_IF_SUCCESS(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, top);
    CONTINUE_IF_SUCCESS(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, bottom);
    CONTINUE_IF_SUCCESS(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, left);
    CONTINUE_IF_SUCCESS(GL_TEXTURE_CUBE_MAP_POSITIVE_X, right);
#   undef CONTINUE_IF_SUCCESS
    FORCE_ASSERT_GL_ERROR();

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    FORCE_ASSERT_GL_ERROR();
  }

  int LoadedTexture::loadCubeSide(GLenum side_target, const std::string file_name) {
    int x, y, n;
    int force_channels = 4;
    unsigned char *image_data = stbi_load(file_name.c_str(), &x, &y, &n, force_channels);
    if (!image_data) {
      fprintf(stderr, "Failed to load texture file: '%s'.\n", file_name);
      return 1;
    }
    // non-power-of-2 dimensions check
    if ((x & (x - 1)) != 0 || (y & (y - 1)) != 0) {
      fprintf(stderr, "Texture not power of two: '%s'.\n", file_name);
      return 1;
    }
    // copy image data into 'target' side of cube map
    glTexImage2D(side_target, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);
    return 0;
  }

  LoadedTexture::LoadedTexture(const std::string &textureFile, RequestedType requestedType) {
    switch (requestedType) {
      case SINGLE: {
        printf("Loading texture: %s\n", textureFile.c_str());
        loadSingleTexture(textureFile);
      } break;
      case CUBE: {
        printf("Loading cube map: %s\n", textureFile.c_str());
        loadCubeTexture(textureFile);
      } break;
      default: break;
    }
  }

  LoadedTexture::~LoadedTexture() {
    glDeleteTextures(1, &mTexture);
  }

  GLuint LoadedTexture::get() {
    return mTexture;
  }
}
