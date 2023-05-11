// run this function when the document is loaded
window.onload = function () {
    // Start up uibuilder - see the docs for the optional parameters
    uibuilder.start()

    // uibuilder.send({
    //     'topic': 'cart',
    //     'payload': null,
    // })

    // Listen for incoming messages from Node-RED and action
    uibuilder.onChange('msg', (msg) => {
        // console.log(msg);
        if (msg.topic == "products") {
            showProducts(msg)
        }
    })
}

const showProducts = (msg) => {
    let outputHTML = msg.payload.map(product => {
        return `
        <div class="list-item">
            <div class="list-item-image">
                <figure class="image is-96x96">
                    <img src="${product.image}"
                        alt="${product.name}">
                </figure>
            </div>
    
            <div class="list-item-content">
                <div class="list-item-title">${product.name}</div>
            </div>
    
            <div class="list-item-controls">
                <div class="is-flex is-align-items-center">
                    <span>RS: ${product.price}</span>
    
                    <div class="control ml-3 is-hidden-mobile">
                        <input class="input" type="text" value="${product.quantity}" disabled>
                    </div>
                </div>
            </div>
        </div>
        `
    }).join('')

    document.getElementById('productsList').innerHTML = outputHTML
}
