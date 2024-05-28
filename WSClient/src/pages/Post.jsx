import 'react';
import postreq from '../common/posts';
const Post = ({ post, index, maxIndex, setIndex, setPost, posts, setCounter }) => {

    const likeDislike = () => {
        postreq("/like", {postId: post.id, userId: parseInt(localStorage.getItem("id"))});
    }

    const ImageComponent = () => {
        const base64String = btoa(
            post.image.reduce((data, byte) => data + String.fromCharCode(byte), '')
        );

        return (
            <img style={{ width: 300 }}
                src={`data:image/jpeg;base64,${base64String}`}
                alt="Image"
            />
        );
    }
    return <div style={{ width: "100vw", height: "100vh", display: 'flex', justifyContent: 'center', alignItems: 'center' }}>
        <div style={{ width: 1000, height: 800, backgroundColor: 'white', border: '5px black solid' }}>
            <div style={{ position: 'relative', top: 0 }}>
                <button style={{ backgroundColor: 'rgb(255, 192, 203)', color: 'white' }} onClick={() => setCounter(prev => prev + 1)}>Refresh</button>
            </div>
            <div style={{ width: '100%', textAlign: 'center', fontSize: 36 }}>
                {post.userName}
            </div>
            <div style={{ display: 'flex', width: '100%' }}>
                <div style={{ width: '20%', height: "100%", textAlign: 'center' }}>
                    <div>
                        <button disabled={index == 0} style={{ backgroundColor: 'black', color: 'white' }} onClick={() => setIndex(0)}>{"<<"}</button>
                        <button disabled={index == 0} style={{ backgroundColor: 'red', color: 'white', position: 'absolute', left: "30%", top: "40%", width: 80 }} onClick={() => setIndex(index - 1)}>{"<"}</button>
                    </div>
                </div>                <div style={{ width: '60%', textAlign: 'center', fontSize: 36 }}>
                    <ImageComponent />
                </div>
                <div style={{ width: '20%', height: "100%", textAlign: 'center' }}>
                    <div>
                        <button disabled={index == maxIndex} style={{ backgroundColor: 'black', color: 'white' }} onClick={() => setIndex(maxIndex)}>{">>"}</button>
                        <button disabled={index == maxIndex} style={{ backgroundColor: 'red', color: 'white', position: 'absolute', right: "30%", top: "40%", width: 80 }} onClick={() => setIndex(index + 1)}>{">"}</button>
                    </div>
                </div>
            </div>
            <div style={{ textAlign: 'center', width: '100%', fontSize: 24 }}>
                {post.description}
            </div>
            <div style={{ display: 'flex', alignContent: 'center', justifyContent: 'center', width: '100%', marginTop: 10 }}>
                <div style={{ width: 200, display: 'flex', alignContent: 'baseline', justifyContent: 'space-around', alignItems: 'baseline' }}>
                    <div>
                        <button onClick={() => {
                            if(post.liked) {
                                const newPost = new Object({ ...post, liked: false, likeCount: post.likeCount - 1 });
                                posts[index] = newPost;
                                 setPost((post) => newPost);
                            } else {
                                const newPost = new Object({ ...post, liked: true, likeCount: post.likeCount + 1 });
                                posts[index] = newPost;
                                setPost((post) => newPost);
                            }
                            likeDislike();
                        }}>Like</button>
                    </div>
                    <b fontSize={24}>{post.likeCount}</b>
                </div>
            </div>
            <div style={{ position: 'relative', bottom: 0, left: "460px", marginTop: 20 }}>
                <button style={{ backgroundColor: 'rgb(0, 255, 0)' }} >New Post</button>
            </div>
        </div>
    </div>
}

export default Post;