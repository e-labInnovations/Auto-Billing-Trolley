// run this function when the document is loaded
window.onload = function () {
    // Start up uibuilder - see the docs for the optional parameters
    uibuilder.start()

    uibuilder.send({
        'topic': 'cartItems',
        'payload': null,
    })

    // Listen for incoming messages from Node-RED and action
    uibuilder.onChange('msg', (msg) => {
        // console.log("🚀 ~ file: index.js:17 ~ uibuilder.onChange ~ msg:", msg)
        if (msg.topic == "cartItems") {
            showCartItems(msg)
        } else if(msg.topic == "cartWarning") {
            bulmaToast.toast({ message: msg.payload, type: 'is-danger' })
        }
    })
}
const showCartItems = (msg) => {
    let totalPrice = 0
    let totalItems = msg.payload.length
    let outputHTML = msg.payload.map(product => {
        totalPrice += product.price
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
                    <span>Rs. ${product.price.toFixed(2)}</span>
    
                    <div class="control ml-3 is-hidden-mobile">
                        <input class="input" type="text" value="${product.quantity}" disabled>
                    </div>
                </div>
            </div>
        </div>
        `
    }).join('')

    document.getElementById('productsList').innerHTML = outputHTML
    document.getElementById('totalPrice').innerHTML = 'Rs. ' + totalPrice.toFixed(2)
    document.getElementById('totalItems').innerHTML = totalItems
}
