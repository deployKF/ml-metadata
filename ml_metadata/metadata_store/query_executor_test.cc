/* Copyright 2022 Google LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    https://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#include "ml_metadata/metadata_store/query_executor_test.h"

#include <cstddef>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "absl/status/status.h"
#include "absl/time/clock.h"
#include "absl/types/optional.h"
#include "ml_metadata/metadata_store/constants.h"
#include "ml_metadata/metadata_store/test_util.h"
#include "ml_metadata/proto/metadata_source.pb.h"
#include "ml_metadata/proto/metadata_store.pb.h"

namespace ml_metadata {
namespace testing {
using ::testing::SizeIs;

// A utility macros for OSS files as ASSERT_OK is not available in OSS.
#define MLMD_ASSERT_OK(expr) ASSERT_EQ(absl::OkStatus(), expr)

constexpr absl::string_view kArtifactTypeRecordSet =
    R"pb(column_names: "id"
         column_names: "name"
         column_names: "version"
         column_names: "description"
         records {
           values: "1"
           values: "artifact_type_1"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
         }
         records {
           values: "2"
           values: "artifact_type_2"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
         }
    )pb";
constexpr absl::string_view kExecutionTypeRecordSet =
    R"pb(column_names: "id"
         column_names: "name"
         column_names: "version"
         column_names: "description"
         records {
           values: "3"
           values: "execution_type_1"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
         }
         records {
           values: "4"
           values: "execution_type_2"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
         }
    )pb";
constexpr absl::string_view kContextTypeRecordSet =
    R"pb(column_names: "id"
         column_names: "name"
         column_names: "version"
         column_names: "description"
         records {
           values: "5"
           values: "context_type_1"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
         }
    )pb";
constexpr absl::string_view kArtifactTypeRecordSet2 =
    R"pb(column_names: "id"
         column_names: "name"
         column_names: "version"
         column_names: "description"
         column_names: "input_type"
         column_names: "output_type"
         records {
           values: "1"
           values: "artifact_type_1"
           values: "version_1"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
         }
         records {
           values: "2"
           values: "artifact_type_2"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
         }
    )pb";
constexpr absl::string_view kExecutionTypeRecordSet2 =
    R"pb(column_names: "id"
         column_names: "name"
         column_names: "version"
         column_names: "description"
         column_names: "input_type"
         column_names: "output_type"
         records {
           values: "3"
           values: "execution_type_1"
           values: "version_1"
           values: "__MLMD_NULL__"
           values: "{\"any\":{}}"
           values: "{\"none\":{}}"
         }
         records {
           values: "4"
           values: "execution_type_2"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
           values: "{\"any\":{}}"
           values: "{\"none\":{}}"
         }
    )pb";
constexpr absl::string_view kContextTypeRecordSet2 =
    R"pb(column_names: "id"
         column_names: "name"
         column_names: "version"
         column_names: "description"
         column_names: "input_type"
         column_names: "output_type"
         records {
           values: "5"
           values: "context_type_1"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
           values: "__MLMD_NULL__"
         }
    )pb";

int GetColumnIndex(const RecordSet& record_set, const std::string column_name) {
  int id_column_index = -1;
  for (int i = 0; i < record_set.column_names_size(); ++i) {
    if (record_set.column_names()[i] == column_name) {
      id_column_index = i;
      break;
    }
  }
  return id_column_index;
}

TEST_P(QueryExecutorTest, SelectTypesByID) {
  ASSERT_EQ(absl::OkStatus(), Init());
  // Artifact type insertion.
  int64 type_id_1, type_id_2;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertArtifactType(
                "artifact_type_1", absl::nullopt, absl::nullopt,
                &type_id_1));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertArtifactType(
                "artifact_type_2", absl::nullopt, absl::nullopt,
                &type_id_2));
  // Exectuion type insertion.
  int64 type_id_3, type_id_4;
  ArtifactStructType input_type;
  AnyArtifactStructType any_input_type;
  *input_type.mutable_any() = any_input_type;
  ArtifactStructType output_type;
  NoneArtifactStructType none_input_type;
  *output_type.mutable_none() = none_input_type;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertExecutionType(
                "execution_type_1", absl::nullopt, absl::nullopt, &input_type,
                &output_type,
                &type_id_3));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertExecutionType(
                "execution_type_2", absl::nullopt, absl::nullopt, &input_type,
                &output_type,
                &type_id_4));
  // Context type insertion.
  int64 type_id_5;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertContextType(
                "context_type_1", absl::nullopt, absl::nullopt,
                &type_id_5));

  // Test select artifact types by ids.
  TypeKind type_kind = TypeKind::ARTIFACT_TYPE;
  std::vector<int64> type_ids = {type_id_1, type_id_2};
  RecordSet artifact_record_set;
  ASSERT_EQ(absl::OkStatus(), query_executor_->SelectTypesByID(
                                  type_ids, type_kind, &artifact_record_set));
  RecordSet expected_record_set = testing::ParseTextProtoOrDie<RecordSet>(
      std::string(kArtifactTypeRecordSet));
  EXPECT_THAT(artifact_record_set, testing::EqualsProto(expected_record_set));

  // Test select execution types by ids.
  type_kind = TypeKind::EXECUTION_TYPE;
  type_ids = {type_id_3, type_id_4};
  RecordSet execution_record_set;
  ASSERT_EQ(absl::OkStatus(), query_executor_->SelectTypesByID(
                                  type_ids, type_kind, &execution_record_set));
  expected_record_set = testing::ParseTextProtoOrDie<RecordSet>(
      std::string(kExecutionTypeRecordSet));
  EXPECT_THAT(execution_record_set, testing::EqualsProto(expected_record_set));

  // Test select context types by ids.
  type_kind = TypeKind::CONTEXT_TYPE;
  type_ids = {type_id_5};
  RecordSet context_record_set;
  ASSERT_EQ(absl::OkStatus(), query_executor_->SelectTypesByID(
                                  type_ids, type_kind, &context_record_set));
  expected_record_set = testing::ParseTextProtoOrDie<RecordSet>(
      std::string(kContextTypeRecordSet));
  EXPECT_THAT(context_record_set, testing::EqualsProto(expected_record_set));
}

TEST_P(QueryExecutorTest, SelectTypesByIDWithMixedTypeIDKinds) {
  ASSERT_EQ(absl::OkStatus(), Init());
  // Artifact type insertion.
  int64 type_id_1, type_id_2;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertArtifactType(
                "artifact_type_1", absl::nullopt, absl::nullopt,
                &type_id_1));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertArtifactType(
                "artifact_type_2", absl::nullopt, absl::nullopt,
                &type_id_2));
  // Context type insertion.
  int64 type_id_3;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertContextType(
                "context_type_1", absl::nullopt, absl::nullopt,
                &type_id_3));

  // Test select artifact types with a mixture of artifact and context type ids.
  TypeKind type_kind = TypeKind::ARTIFACT_TYPE;
  std::vector<int64> type_ids = {type_id_1, type_id_3};
  RecordSet record_set;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->SelectTypesByID(type_ids, type_kind, &record_set));
  // Verify that only artifact with `type_id_1` is retrieved.
  ASSERT_EQ(record_set.records_size(), 1);
  EXPECT_EQ(record_set.records(0).values(1), "artifact_type_1");
}

TEST_P(QueryExecutorTest, DeleteContextsById) {
  ASSERT_EQ(absl::OkStatus(), Init());
  // Create context type.
  int64 context_type_id;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertContextType(
                "context_type", absl::nullopt, absl::nullopt,
                &context_type_id));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertTypeProperty(context_type_id, "property_1",
                                                PropertyType::INT));
  // Create artifact type.
  int64 artifact_type_id;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertArtifactType(
                "artifact_type", absl::nullopt, absl::nullopt,
                &artifact_type_id));
  // Create execution type.
  int64 execution_type_id;
  ArtifactStructType input_type;
  AnyArtifactStructType any_input_type;
  *input_type.mutable_any() = any_input_type;
  ArtifactStructType output_type;
  NoneArtifactStructType none_input_type;
  *output_type.mutable_none() = none_input_type;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertExecutionType(
                "execution_type", absl::nullopt, absl::nullopt, &input_type,
                &output_type,
                &execution_type_id));

  // Create contexts.
  int64 context_id_1, context_id_2;
  MLMD_ASSERT_OK(query_executor_->InsertContext(
      context_type_id, "delete_contexts_by_id_test_1",
      absl::Now(), absl::Now(), &context_id_1));
  MLMD_ASSERT_OK(query_executor_->InsertContext(
      context_type_id, "delete_contexts_by_id_test_2",
      absl::Now(), absl::Now(), &context_id_2));
  Value int_value;
  int_value.set_int_value(3);
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertContextProperty(context_id_1, "property_1",
                                                   false, int_value));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertContextProperty(context_id_2, "property_1",
                                                   false, int_value));
  // Create artifact and execution.
  int64 artifact_id, execution_id;
  MLMD_ASSERT_OK(query_executor_->InsertArtifact(
      artifact_type_id, "/foo/bar", absl::nullopt, "artifact",
      absl::Now(), absl::Now(), &artifact_id));
  MLMD_ASSERT_OK(query_executor_->InsertExecution(
      execution_type_id, absl::nullopt, "execution",
      absl::Now(), absl::Now(), &execution_id));

  // Create attribution and association.
  int64 attribution_id, association_id;
  ASSERT_EQ(absl::OkStatus(), query_executor_->InsertAttributionDirect(
                                  context_id_1, artifact_id, &attribution_id));
  ASSERT_EQ(absl::OkStatus(), query_executor_->InsertAssociation(
                                  context_id_1, execution_id, &association_id));

  ASSERT_EQ(absl::OkStatus(), AddCommitPointIfNeeded());

  // Test: actual deletion on context1
  {
    ASSERT_EQ(absl::OkStatus(),
              query_executor_->DeleteContextsById({context_id_1}));
    RecordSet record_set;
    ASSERT_EQ(absl::OkStatus(), query_executor_->SelectContextsByID(
                                    {context_id_1, context_id_2}, &record_set));

    // Verify: context1 was deleted; context2 still remains.
    ASSERT_EQ(record_set.records_size(), 1);
    const int id_column_index = GetColumnIndex(record_set, "id");
    ASSERT_GE(id_column_index, 0);
    EXPECT_EQ(record_set.records(0).values(id_column_index),
              std::to_string(context_id_2));

    // Verify: context properties for context1 were also deleted.
    RecordSet property_record_set;
    ASSERT_EQ(absl::OkStatus(),
              query_executor_->SelectContextPropertyByContextID(
                  {context_id_1}, &property_record_set));
    EXPECT_EQ(property_record_set.records_size(), 0);

    // Verify: arrtibution and association for context1 were not deleted.
    RecordSet attribution_set, association_set;
    ASSERT_EQ(absl::OkStatus(), query_executor_->SelectAttributionByContextID(
                                    context_id_1, &attribution_set));
    EXPECT_EQ(attribution_set.records_size(), 1);
    ASSERT_EQ(absl::OkStatus(), query_executor_->SelectAssociationByContextIDs(
                                    {context_id_1}, &association_set));
    EXPECT_EQ(association_set.records_size(), 1);
  }
  // Test: context id was wrong when deleting context2
  {
    // Still returns OK status when `context_id_2 + 1` is not found.
    ASSERT_EQ(absl::OkStatus(),
              query_executor_->DeleteContextsById({context_id_2 + 1}));
    RecordSet record_set;
    ASSERT_EQ(absl::OkStatus(),
              query_executor_->SelectContextsByID({context_id_2}, &record_set));

    // Verify: context2 remains because context id was wrong when deleting it.
    ASSERT_EQ(record_set.records_size(), 1);
    const int id_column_index = GetColumnIndex(record_set, "id");
    ASSERT_GE(id_column_index, 0);
    EXPECT_EQ(record_set.records(0).values(id_column_index),
              std::to_string(context_id_2));

    // Verify: context properties for context2 also remain.
    RecordSet property_record_set;
    ASSERT_EQ(absl::OkStatus(),
              query_executor_->SelectContextPropertyByContextID(
                  {context_id_2}, &property_record_set));
    EXPECT_EQ(property_record_set.records_size(), 1);
  }
}

TEST_P(QueryExecutorTest, DeleteParentContextsByParentIdAndChildIds) {
  MLMD_ASSERT_OK(Init());
  // Create context type.
  int64 context_type_id;
  MLMD_ASSERT_OK(query_executor_->InsertContextType(
      "context_type", absl::nullopt, absl::nullopt,
      &context_type_id));
  MLMD_ASSERT_OK(query_executor_->InsertTypeProperty(context_type_id,
                                  "property_1", PropertyType::INT));

  // Create contexts
  int64 parent_id_1, parent_id_2, parent_id_3;
  int64 child_id_1, child_id_2, child_id_3;
  MLMD_ASSERT_OK(query_executor_->InsertContext(
      context_type_id, "parent_context_1",
      absl::Now(), absl::Now(), &parent_id_1));
  MLMD_ASSERT_OK(query_executor_->InsertContext(
      context_type_id, "parent_context_2",
      absl::Now(), absl::Now(), &parent_id_2));
  MLMD_ASSERT_OK(query_executor_->InsertContext(
      context_type_id, "parent_context_3",
      absl::Now(), absl::Now(), &parent_id_3));
  MLMD_ASSERT_OK(query_executor_->InsertContext(
      context_type_id, "child_context_1",
      absl::Now(), absl::Now(), &child_id_1));
  MLMD_ASSERT_OK(query_executor_->InsertContext(
      context_type_id, "child_context_2",
      absl::Now(), absl::Now(), &child_id_2));
  MLMD_ASSERT_OK(query_executor_->InsertContext(
      context_type_id, "child_context_3",
      absl::Now(), absl::Now(), &child_id_3));

  // Create Parent Context
  // parent context 1 has three child contexts:
  // {child_id_1, child_id_2, child_id_3}
  // parent context 2 has one child contexts: {child_id_1}
  // parent context 3 has no child id (independent)
  MLMD_ASSERT_OK(query_executor_->InsertParentContext(parent_id_1, child_id_1));
  MLMD_ASSERT_OK(query_executor_->InsertParentContext(parent_id_1, child_id_2));
  MLMD_ASSERT_OK(query_executor_->InsertParentContext(parent_id_1, child_id_3));
  MLMD_ASSERT_OK(query_executor_->InsertParentContext(parent_id_2, child_id_1));

  // Test: independent context
  {
    // Still returns OK status when `parent_id_3` is not found
    // in CheckParentContextTable
    MLMD_ASSERT_OK(query_executor_->
        DeleteParentContextsByParentIdAndChildIds(parent_id_3, {child_id_1}));
    RecordSet record_set;
    // Verify: child context 1 still remains
    MLMD_ASSERT_OK(query_executor_->
        SelectParentContextsByContextID(child_id_1, &record_set));
    EXPECT_THAT(record_set.records(), SizeIs(2));
  }

  // Test: actual delete child context 1 from parent context 1
  {
    // Remove child context 1 and child context 2 from parent context 1
    MLMD_ASSERT_OK(query_executor_->
        DeleteParentContextsByParentIdAndChildIds(
            parent_id_1, {child_id_1, child_id_2}));

    RecordSet record_set_1;
    // Verify: parent context 1 still has one record in the table
    MLMD_ASSERT_OK(query_executor_->
        SelectChildContextsByContextID(parent_id_1, &record_set_1));
    ASSERT_THAT(record_set_1.records(), SizeIs(1));
    // Find the index for column "context_id" and "parent_context_id"
    const int child_id_column_index_1 =
        GetColumnIndex(record_set_1, "context_id");
    ASSERT_GE(child_id_column_index_1, 0);
    const int parent_id_column_index_1 =
        GetColumnIndex(record_set_1, "parent_context_id");
    ASSERT_GE(parent_id_column_index_1, 0);
    // Verify: child context 2 remains as child context of parent context 1
    EXPECT_EQ(record_set_1.records(0).values(child_id_column_index_1),
        absl::StrCat(child_id_3));
    EXPECT_EQ(record_set_1.records(0).values(parent_id_column_index_1),
        absl::StrCat(parent_id_1));

    RecordSet record_set_2;
    // Verify: parent context 2 still has one record in the table
    MLMD_ASSERT_OK(query_executor_->
        SelectChildContextsByContextID(parent_id_2, &record_set_2));
    ASSERT_THAT(record_set_2.records(), SizeIs(1));
    // Find the index for column "context_id" and "parent_context_id"
    const int child_id_column_index_2 =
        GetColumnIndex(record_set_2, "context_id");
    ASSERT_GE(child_id_column_index_2, 0);
    const int parent_id_column_index_2 =
        GetColumnIndex(record_set_2, "parent_context_id");
    ASSERT_GE(parent_id_column_index_2, 0);
    // Verify: child context 1 remains as child context of parent context 2
    EXPECT_EQ(record_set_2.records(0).values(child_id_column_index_2),
        absl::StrCat(child_id_1));
    EXPECT_EQ(record_set_2.records(0).values(parent_id_column_index_2),
        absl::StrCat(parent_id_2));
  }
}

TEST_P(QueryExecutorTest, SelectParentTypesByTypeID) {
  ASSERT_EQ(absl::OkStatus(), Init());
  // Setup: Create context type.
  int64 context_type_id;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertContextType(
                "context_type", absl::nullopt, absl::nullopt,
                &context_type_id));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertTypeProperty(context_type_id, "property_1",
                                                PropertyType::INT));
  // Create artifact types.
  int64 artifact_type_id, parent_artifact_type_id;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertArtifactType(
                "artifact_type", absl::nullopt, absl::nullopt,
                &artifact_type_id));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertArtifactType(
                "parent_artifact_type", absl::nullopt, absl::nullopt,
                &parent_artifact_type_id));

  // Setup: Create execution types.
  int64 execution_type_id, parent_execution_type_id;
  ArtifactStructType input_type;
  AnyArtifactStructType any_input_type;
  *input_type.mutable_any() = any_input_type;
  ArtifactStructType output_type;
  NoneArtifactStructType none_input_type;
  *output_type.mutable_none() = none_input_type;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertExecutionType(
                "execution_type", absl::nullopt, absl::nullopt, &input_type,
                &output_type,
                &execution_type_id));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertExecutionType(
                "parent_execution_type", absl::nullopt, absl::nullopt,
                &input_type, &output_type,
                &parent_execution_type_id));
  int64 non_exist_parent_type_id = parent_execution_type_id + execution_type_id;

  // Setup: Insert parent type links.
  ASSERT_EQ(absl::OkStatus(), query_executor_->InsertParentType(
                                  artifact_type_id, parent_artifact_type_id));
  ASSERT_EQ(absl::OkStatus(), query_executor_->InsertParentType(
                                  execution_type_id, parent_execution_type_id));
  ASSERT_EQ(absl::OkStatus(), query_executor_->InsertParentType(
                                  execution_type_id, non_exist_parent_type_id));

  // Test: select parent type ids for a type without parent types.
  {
    RecordSet record_set;
    ASSERT_EQ(absl::OkStatus(), query_executor_->SelectParentTypesByTypeID(
                                    {context_type_id}, &record_set));
    EXPECT_EQ(record_set.records_size(), 0);
  }
  // Test: select a parent type that does not exist.
  {
    RecordSet record_set;
    ASSERT_EQ(absl::OkStatus(), query_executor_->SelectParentTypesByTypeID(
                                    {execution_type_id}, &record_set));
    ASSERT_EQ(record_set.records_size(), 2);
    EXPECT_EQ(record_set.records(0).values(0),
              std::to_string(execution_type_id));
    EXPECT_EQ(record_set.records(0).values(1),
              std::to_string(parent_execution_type_id));
    // Verify: the record is still returned although the type does not exist
    // because it only stores type ids.
    EXPECT_EQ(record_set.records(1).values(0),
              std::to_string(execution_type_id));
    EXPECT_EQ(record_set.records(1).values(1),
              std::to_string(non_exist_parent_type_id));
  }
  // Test: select parent type ids for a mixture of context, artifact and
  // execution type ids.
  {
    RecordSet record_set;
    ASSERT_EQ(absl::OkStatus(),
              query_executor_->SelectParentTypesByTypeID(
                  {context_type_id, artifact_type_id, execution_type_id},
                  &record_set));
    // Verify: SelectParentTypesByTypeID can return a mixture of different type
    // kinds because it only stores type ids.
    ASSERT_EQ(record_set.records_size(), 3);
    EXPECT_EQ(record_set.records(0).values(0),
              std::to_string(artifact_type_id));
    EXPECT_EQ(record_set.records(0).values(1),
              std::to_string(parent_artifact_type_id));
    EXPECT_EQ(record_set.records(1).values(0),
              std::to_string(execution_type_id));
    EXPECT_EQ(record_set.records(1).values(1),
              std::to_string(parent_execution_type_id));
    EXPECT_EQ(record_set.records(2).values(0),
              std::to_string(execution_type_id));
    EXPECT_EQ(record_set.records(2).values(1),
              std::to_string(non_exist_parent_type_id));
  }
}

TEST_P(QueryExecutorTest, SelectPropertiesByTypeID) {
  ASSERT_EQ(absl::OkStatus(), Init());
  // Setup: Create context type.
  int64 context_type_id;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertContextType(
                "context_type", absl::nullopt, absl::nullopt,
                &context_type_id));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertTypeProperty(context_type_id, "property_1",
                                                PropertyType::INT));
  // Create artifact types.
  int64 artifact_type_id_1, artifact_type_id_2;
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertArtifactType(
                "artifact_type_1", absl::nullopt, absl::nullopt,
                &artifact_type_id_1));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertTypeProperty(
                artifact_type_id_1, "property_1", PropertyType::INT));
  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertTypeProperty(
                artifact_type_id_1, "property_2", PropertyType::STRING));

  ASSERT_EQ(absl::OkStatus(),
            query_executor_->InsertArtifactType(
                "artifact_type_2", absl::nullopt, absl::nullopt,
                &artifact_type_id_2));

  // Test: select a type with no type properties.
  {
    RecordSet record_set;
    ASSERT_EQ(absl::OkStatus(), query_executor_->SelectPropertiesByTypeID(
                                    {artifact_type_id_2}, &record_set));
    ASSERT_EQ(record_set.records_size(), 0);
  }

  // Test: select properties for multiple type ids.
  {
    RecordSet record_set;
    ASSERT_EQ(absl::OkStatus(),
              query_executor_->SelectPropertiesByTypeID(
                  {artifact_type_id_1, artifact_type_id_2}, &record_set));
    ASSERT_EQ(record_set.records_size(), 2);
    EXPECT_EQ(record_set.records(0).values(0),
              std::to_string(artifact_type_id_1));
    EXPECT_EQ(record_set.records(0).values(1), "property_1");
    EXPECT_EQ(record_set.records(0).values(2),
              std::to_string(PropertyType::INT));

    EXPECT_EQ(record_set.records(1).values(0),
              std::to_string(artifact_type_id_1));
    EXPECT_EQ(record_set.records(1).values(1), "property_2");
    EXPECT_EQ(record_set.records(1).values(2),
              std::to_string(PropertyType::STRING));
  }
  // Test: select properties for type ids of a mixture of context and artifact
  // types.
  {
    RecordSet record_set;
    ASSERT_EQ(absl::OkStatus(),
              query_executor_->SelectPropertiesByTypeID(
                  {context_type_id, artifact_type_id_1, artifact_type_id_2},
                  &record_set));
    // Verify: SelectPropertiesByTypeID can return a mixture of different type
    // kinds because it only stores type ids.
    ASSERT_EQ(record_set.records_size(), 3);
    EXPECT_EQ(record_set.records(0).values(0), std::to_string(context_type_id));
    EXPECT_EQ(record_set.records(0).values(1), "property_1");
    EXPECT_EQ(record_set.records(0).values(2),
              std::to_string(PropertyType::INT));

    EXPECT_EQ(record_set.records(1).values(0),
              std::to_string(artifact_type_id_1));
    EXPECT_EQ(record_set.records(1).values(1), "property_1");
    EXPECT_EQ(record_set.records(1).values(2),
              std::to_string(PropertyType::INT));

    EXPECT_EQ(record_set.records(2).values(0),
              std::to_string(artifact_type_id_1));
    EXPECT_EQ(record_set.records(2).values(1), "property_2");
    EXPECT_EQ(record_set.records(2).values(2),
              std::to_string(PropertyType::STRING));
  }
}

TEST_P(QueryExecutorTest, SelectTypesByNamesAndVersions) {
  ASSERT_EQ(Init(), absl::OkStatus());
  // Artifact type insertion.
  int64 type_id_1, type_id_2;
  ASSERT_EQ(query_executor_->InsertArtifactType(
                "artifact_type_1", "version_1", absl::nullopt,
                &type_id_1),
            absl::OkStatus());
  ASSERT_EQ(query_executor_->InsertArtifactType(
                "artifact_type_2", absl::nullopt, absl::nullopt,
                &type_id_2),
            absl::OkStatus());
  // Exectuion type insertion.
  int64 type_id_3, type_id_4;
  ArtifactStructType input_type;
  AnyArtifactStructType any_input_type;
  *input_type.mutable_any() = any_input_type;
  ArtifactStructType output_type;
  NoneArtifactStructType none_input_type;
  *output_type.mutable_none() = none_input_type;
  ASSERT_EQ(query_executor_->InsertExecutionType(
                "execution_type_1", "version_1", absl::nullopt, &input_type,
                &output_type,
                &type_id_3),
            absl::OkStatus());
  ASSERT_EQ(query_executor_->InsertExecutionType(
                "execution_type_2", absl::nullopt, absl::nullopt, &input_type,
                &output_type,
                &type_id_4),
            absl::OkStatus());
  // Context type insertion.
  int64 type_id_5;
  ASSERT_EQ(query_executor_->InsertContextType(
                "context_type_1", absl::nullopt, absl::nullopt,
                &type_id_5),
            absl::OkStatus());

  auto verify_record_set_with_unordered_records =
      [](RecordSet got_record_set, RecordSet expected_record_set) {
        ASSERT_THAT(got_record_set.column_names(),
                    SizeIs(expected_record_set.column_names().size()));
        EXPECT_THAT(got_record_set.column_names(),
                    ::testing::Pointwise(::testing::Eq(),
                                         got_record_set.column_names()));
        ASSERT_THAT(got_record_set.records(),
                    SizeIs(expected_record_set.records().size()));
        EXPECT_THAT(got_record_set.records(),
                    ::testing::UnorderedPointwise(
                        testing::EqualsProto<RecordSet::Record>(),
                        got_record_set.records()));
      };

  // Test select artifact types by ids.
  TypeKind type_kind = TypeKind::ARTIFACT_TYPE;
  std::vector<std::pair<std::string, std::string>> names_and_versions = {
      {"artifact_type_1", "version_1"}, {"artifact_type_2", ""}};

  RecordSet expected_record_set = testing::ParseTextProtoOrDie<RecordSet>(
      std::string(kArtifactTypeRecordSet2));
  RecordSet artifact_record_set;
  ASSERT_EQ(
      query_executor_->SelectTypesByNamesAndVersions(
          absl::MakeSpan(names_and_versions), type_kind, &artifact_record_set),
      absl::OkStatus());
  verify_record_set_with_unordered_records(artifact_record_set,
                                           expected_record_set);

  // Test select execution types by ids.
  type_kind = TypeKind::EXECUTION_TYPE;
  names_and_versions = {{"execution_type_1", "version_1"},
                        {"execution_type_2", ""}};
  RecordSet execution_record_set;
  ASSERT_EQ(
      query_executor_->SelectTypesByNamesAndVersions(
          absl::MakeSpan(names_and_versions), type_kind, &execution_record_set),
      absl::OkStatus());
  expected_record_set = testing::ParseTextProtoOrDie<RecordSet>(
      std::string(kExecutionTypeRecordSet2));
  verify_record_set_with_unordered_records(execution_record_set,
                                           expected_record_set);

  // Test select context types by ids.
  type_kind = TypeKind::CONTEXT_TYPE;
  names_and_versions = {{"context_type_1", ""}};
  RecordSet context_record_set;
  ASSERT_EQ(
      query_executor_->SelectTypesByNamesAndVersions(
          absl::MakeSpan(names_and_versions), type_kind, &context_record_set),
      absl::OkStatus());
  expected_record_set = testing::ParseTextProtoOrDie<RecordSet>(
      std::string(kContextTypeRecordSet2));
  verify_record_set_with_unordered_records(context_record_set,
                                           expected_record_set);
}

}  // namespace testing
}  // namespace ml_metadata
