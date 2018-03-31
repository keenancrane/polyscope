#include "polyscope/point_cloud_vector_quantity.h"

#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/vector_shaders.h"
#include "polyscope/polyscope.h"

#include "imgui.h"

#include "Eigen/Dense"


using std::cout;
using std::endl;

namespace polyscope {

PointCloudVectorQuantity::PointCloudVectorQuantity(std::string name, std::vector<Vector3> vectors_,
                                                   PointCloud* pointCloud_, VectorType vectorType_)

    : PointCloudQuantity(name, pointCloud_), vectorType(vectorType_), vectors(vectors_) {

  if (vectors.size() != parent->points.size()) {
    polyscope::error("Point cloud vector quantity " + name + " does not have same number of values (" +
                     std::to_string(vectors.size()) + ") as point cloud size (" +
                     std::to_string(parent->points.size()) + ")");
  }

  // Create a mapper (default mapper is identity)
  if (vectorType == VectorType::AMBIENT) {
    mapper.setMinMax(vectors);
  } else {
    mapper = AffineRemapper<Vector3>(vectors, DataType::MAGNITUDE);
  }

  // Default viz settings
  if (vectorType != VectorType::AMBIENT) {
    lengthMult = .02;
  } else {
    lengthMult = 1.0;
  }
  radiusMult = .0005;
  vectorColor = parent->colorManager.getNextSubColor(name);
}

PointCloudVectorQuantity::~PointCloudVectorQuantity() { safeDelete(program); }


void PointCloudVectorQuantity::draw() {
  if (!enabled) return;

  if (program == nullptr) prepare();

  // Set uniforms
  glm::mat4 viewMat = view::getCameraViewMatrix();
  program->setUniform("u_viewMatrix", glm::value_ptr(viewMat));

  glm::mat4 projMat = view::getCameraPerspectiveMatrix();
  program->setUniform("u_projMatrix", glm::value_ptr(projMat));

  Vector3 eyePos = view::getCameraWorldPosition();
  program->setUniform("u_eye", eyePos);

  program->setUniform("u_lightCenter", state::center);
  program->setUniform("u_lightDist", 5 * state::lengthScale);
  program->setUniform("u_radius", radiusMult * state::lengthScale);
  program->setUniform("u_color", vectorColor);

  if (vectorType == VectorType::AMBIENT) {
    program->setUniform("u_lengthMult", 1.0);
  } else {
    program->setUniform("u_lengthMult", lengthMult * state::lengthScale);
  }

  program->draw();
}

void PointCloudVectorQuantity::prepare() {
  program = new gl::GLProgram(&PASSTHRU_VECTOR_VERT_SHADER, &VECTOR_GEOM_SHADER, &SHINY_VECTOR_FRAG_SHADER,
                              gl::DrawMode::Points);

  // Fill buffers
  std::vector<Vector3> mappedVectors;
  for (Vector3 v : vectors) {
    mappedVectors.push_back(mapper.map(v));
  }

  program->setAttribute("a_vector", mappedVectors);
  program->setAttribute("a_position", parent->points);
}

void PointCloudVectorQuantity::drawUI() {


  if (ImGui::TreeNode((name + " (vector)").c_str())) {
    ImGui::Checkbox("Enabled", &enabled);
    ImGui::SameLine();
    ImGui::ColorEdit3("Color", (float*)&vectorColor, ImGuiColorEditFlags_NoInputs);

    // Only get to set length for non-ambient vectors
    if (vectorType != VectorType::AMBIENT) {
      ImGui::SliderFloat("Length", &lengthMult, 0.0, .1, "%.5f", 3.);
    }

    ImGui::SliderFloat("Radius", &radiusMult, 0.0, .1, "%.5f", 3.);

    { // Draw max and min magnitude
      ImGui::TextUnformatted(mapper.printBounds().c_str());
    }

    ImGui::TreePop();
  }
}

void PointCloudVectorQuantity::buildInfoGUI(size_t ind) {
  ImGui::TextUnformatted(name.c_str());
  ImGui::NextColumn();

  std::stringstream buffer;
  buffer << vectors[ind];
  ImGui::TextUnformatted(buffer.str().c_str());

  ImGui::NextColumn();
  ImGui::NextColumn();
  ImGui::Text("magnitude: %g", norm(vectors[ind]));
  ImGui::NextColumn();
}


} // namespace polyscope