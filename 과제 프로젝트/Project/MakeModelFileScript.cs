using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System;
using System.IO;
using System.Text;
using UnityEditor;

public class MakeModelFileScript : MonoBehaviour
{
    string folderPath = "ModelBinaryFile";

    void BinaryWriteString(string str, BinaryWriter binaryWriter)
    {
        binaryWriter.Write(str.Length);
        binaryWriter.Write(str.ToCharArray());
    }
    void BinaryWriteVector3(Vector3 vector, BinaryWriter binaryWriter)
    {
        binaryWriter.Write(vector.x);
        binaryWriter.Write(vector.y);
        binaryWriter.Write(vector.z);
    }
    void BinaryWriteVectorUV(Vector2 vector, BinaryWriter binaryWriter)
    {
        binaryWriter.Write(vector.x);
        binaryWriter.Write((float)1.0f - vector.y);
    }
    void BinaryWriteQuat(Quaternion vector, BinaryWriter binaryWriter)
    {
        binaryWriter.Write(vector.x);
        binaryWriter.Write(vector.y);
        binaryWriter.Write(vector.z);
        binaryWriter.Write(vector.w);
    }

    void BinaryWriteColor(Color c, BinaryWriter binaryWriter)
    {
        binaryWriter.Write(c.r);
        binaryWriter.Write(c.g);
        binaryWriter.Write(c.b);
        binaryWriter.Write(c.a);
    }
    void BinaryWriteMatrix(Matrix4x4 matrix, BinaryWriter binaryWriter)
    {
        binaryWriter.Write(matrix.m00);
        binaryWriter.Write(matrix.m10);
        binaryWriter.Write(matrix.m20);
        binaryWriter.Write(matrix.m30);
        binaryWriter.Write(matrix.m01);
        binaryWriter.Write(matrix.m11);
        binaryWriter.Write(matrix.m21);
        binaryWriter.Write(matrix.m31);
        binaryWriter.Write(matrix.m02);
        binaryWriter.Write(matrix.m12);
        binaryWriter.Write(matrix.m22);
        binaryWriter.Write(matrix.m32);
        binaryWriter.Write(matrix.m03);
        binaryWriter.Write(matrix.m13);
        binaryWriter.Write(matrix.m23);
        binaryWriter.Write(matrix.m33);
    }

    void CreateMaterialBinaryFile(Material material, BinaryWriter binaryWriter)
    {
        BinaryWriteString("Material_" + material.name, binaryWriter);

        // ambient(XMFLOAT4)
        Color ambient = new Color(1.0f, 1.0f, 1.0f, 1.0f);
        BinaryWriteColor(ambient, binaryWriter);
        // diffuse(XMFLOAT4)
        if (material.HasProperty("_Color"))
        {
            Color diffuse = material.GetColor("_Color");
            BinaryWriteColor(diffuse, binaryWriter);
        }
        else
        {
            Color diffuse = new Color(0.0f, 0.0f, 0.0f, 1.0f);
            BinaryWriteColor(diffuse, binaryWriter);
        }
        // specular(XMFLOAT4)(specular.w = 반짝임계수)
        if (material.HasProperty("_SpecColor"))
        {
            Color specular = material.GetColor("_SpecColor");
            BinaryWriteColor(specular, binaryWriter);
        }
        else
        {
            Color specular = new Color(1.0f, 1.0f, 1.0f, 10.0f);
            BinaryWriteColor(specular, binaryWriter);
        }
        // emissive(XMFLOAT4)
        if (material.HasProperty("_EmissionColor"))
        {
            Color emission = material.GetColor("_EmissionColor");
            BinaryWriteColor(emission, binaryWriter);
        }
        else
        {
            Color emission = new Color(0.0f, 0.0f, 0.0f, 1.0f);
            BinaryWriteColor(emission, binaryWriter);
        }

        string[] textureTypeName = new string[7] {
            "_MainTex",         // 알베도, 디퓨즈 맵
            "_OcclusionMap",    // 스페큘러맵
            "_BumpMap",         // 노멀맵
            "_MetallicGlossMap",// 메탈릭
            "_EmissionMap",     // 표면 방출 광원 맵
            "_DetailAlbedoMap", //
            "_DetailNormalMap", //
            // _ParallaxMap
            // _DetailMask
        };

        for(int i = 0; i < textureTypeName.Length; i++)
        {
            Texture temp = material.GetTexture(textureTypeName[i]);
            if (temp)
            {
                BinaryWriteString(temp.name + ".dds", binaryWriter);
                Debug.Log(temp.name + ".dds");
            }
            else
            {
                binaryWriter.Write(0);
            }
        }
    }

    void CreateMeshBinaryFile(Mesh mesh, MeshRenderer meshRenderer, BinaryWriter binaryWriter)
    {
        BinaryWriteString("Mesh_" + mesh.name, binaryWriter);
        
        // nVertex(UINT)
        binaryWriter.Write((uint)mesh.vertexCount);
        // nameSize (UINT) / name (string)
        BinaryWriteString("Mesh_"  + mesh.name, binaryWriter);
        // boundingBox (float * 6)
        BinaryWriteVector3(mesh.bounds.center, binaryWriter);
        BinaryWriteVector3(mesh.bounds.extents, binaryWriter);
        // positions (float * 3 * nVertex)
        foreach (Vector3 position in mesh.vertices)
            BinaryWriteVector3(position, binaryWriter);
        // normals (float * 3 * nVertex)
        foreach (Vector3 normal in mesh.normals)
            BinaryWriteVector3(normal, binaryWriter);

        if (mesh.uv.Length != mesh.vertexCount)
            Debug.Log("버그 : uv와 버텍스의 수가 맞지 않음!!!!!");

        // nUV (UINT)
        binaryWriter.Write((uint)mesh.uv.Length);
        // uv (float * 2 * nVertex)
        foreach (Vector2 texcoord in mesh.uv)
            BinaryWriteVectorUV(texcoord, binaryWriter);

        // nSubMesh (UINT)
        Material[] materials = meshRenderer.materials;
        binaryWriter.Write((uint)mesh.subMeshCount);
        for (int i = 0; i < mesh.subMeshCount; i++)
        {
            // nSubMeshIndex (UINT) / subMeshIndex (UINT * nSubMeshIndex)    ....( * nSubMesh )
            int[] subindicies = mesh.GetTriangles(i);
            binaryWriter.Write(subindicies.Length);
            foreach (int index in subindicies)
                binaryWriter.Write(index);
            // materialNameSize(UINT) / materialName(string)
            CreateMaterialBinaryFile(materials[i], binaryWriter);
        }
    }
    void CreateObjectBinaryFile(Transform curObjectTransform, BinaryWriter binaryWriter)
    {
        string objectName = "GameObject_" + curObjectTransform.name;

        // nameSize (UINT) / name(string)
        BinaryWriteString(objectName, binaryWriter);

        //localPosition(float3)
        BinaryWriteVector3(curObjectTransform.localPosition, binaryWriter);

        //localScale(float3)
        BinaryWriteVector3(curObjectTransform.localScale, binaryWriter);

        //localRotation(float4)
        BinaryWriteQuat(curObjectTransform.localRotation, binaryWriter);


        // meshNameSize(UINT) / meshName(string)	=> 메쉬가 없을 경우 따로 처리하자
        MeshFilter meshFilter = curObjectTransform.GetComponent<MeshFilter>();
        MeshRenderer meshRenderer = curObjectTransform.GetComponent<MeshRenderer>();

        if (meshFilter && meshRenderer)    // 메쉬가 있는 경우
        {
            CreateMeshBinaryFile(meshFilter.sharedMesh, meshRenderer, binaryWriter);
            
        }
        else  // 메쉬가 없는 경우
        {
            binaryWriter.Write(0);
        }

        // nChildren(UINT)
        binaryWriter.Write(curObjectTransform.childCount);
        for (int i = 0; i < curObjectTransform.childCount; i++)  // 자식들을 똑같은 포멧으로 저장
        {
            CreateObjectBinaryFile(curObjectTransform.GetChild(i), binaryWriter);
        }
    }

    void MakeModel(Transform _gameObject)
    {
        // 최상위 오브젝트의 이름으로 폴더를 만든다.
        string fileName = _gameObject.name;
        DirectoryInfo directoryInfo = new DirectoryInfo(folderPath);
        if (directoryInfo.Exists == false)
        {
            directoryInfo.Create();
        }

        // 최상위 오브젝트의 이름으로 파일을 만든다.
        BinaryWriter binaryWriter = new BinaryWriter(File.Open(folderPath + "/" + fileName, FileMode.Create));
        CreateObjectBinaryFile(_gameObject.transform, binaryWriter);
        binaryWriter.Flush();
        binaryWriter.Close();
    }
    // Start is called before the first frame update
    void Start()
    {
        for (int i = 0; i < transform.childCount; i++)  // 자식들을 똑같은 포멧으로 저장
        {
            if(transform.GetChild(i).gameObject.activeSelf)
                MakeModel(transform.GetChild(i));
        }
    }

}
